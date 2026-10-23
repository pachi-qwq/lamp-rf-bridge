// SPDX-License-Identifier: MIT
#pragma once
#include <WebServer.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <mbedtls/md.h>
#include <cstdlib>
#include "web_policy.h"
#include "web_config.h"

// Arduino-ESP32 2.0.17 keeps one global Digest nonce; a second client invalidates the first.
// Arduino-ESP32 2.0.17 的 Digest 只保存一份全局 nonce，第二个客户端会使前者失效。
// Sign challenges with a per-boot HMAC and validate independently without patching the pinned framework.
// 本类为挑战值加启动周期的 HMAC，并逐个验证，避免修改固定版本的框架文件。
/**
 * @brief Pinned WebServer adapter for signed independent Digest and idle preconnections.
 * 为固定 WebServer 提供独立签名 Digest 与空闲预连接处理。
 */
class LampWebServer : public WebServer {
 public:
  /**
   * @brief Pinned WebServer adapter for signed independent Digest and idle preconnections.
   * 为固定 WebServer 提供独立签名 Digest 与空闲预连接处理。
   * @param port HTTP listen port, normally web_config::kPort. / HTTP 监听端口，通常为 kPort。
   */
  explicit LampWebServer(uint16_t port) : WebServer(port) {}
  void handleClient() override {
    // The pinned server serves one client; a byte-free browser preconnection can block requests for 5 seconds.
    // 固定框架只服务一条连接。浏览器预连接若不发请求，默认会挡住后面的请求 5 秒。
    // Yield only before the first byte, after 200 ms, and when another client is queued.
    // 仅在首字节尚未到达、已等待 200ms 且另有连接排队时让出入口；
    // Requests with bytes available or already parsing/sending retain framework behavior.
    // 已有请求字节和正在解析／发送的请求保持框架原有行为。
    if (_currentStatus == HC_WAIT_READ &&
        static_cast<uint32_t>(millis() - _statusChange) >= web_config::kIdlePreconnectWaitMs &&
        _currentClient.connected() && !_currentClient.available() && _server.hasClient()) {
      _currentClient.stop();
      _currentClient = WiFiClient();
      _currentStatus = HC_NONE;
      _currentUpload.reset();
      _currentRaw.reset();
      ++idlePreconnectClosed_;
      Serial.printf("{\"type\":\"http_idle_preconnect_closed\",\"count\":%lu}\n",
                    static_cast<unsigned long>(idlePreconnectClosed_));
    }
    WebServer::handleClient();
  }
  /**
   * @brief Return the boot-local count of idle connections closed by the guard.
   * 返回本次启动被保护逻辑关闭的空闲连接数。
   */
  uint32_t idlePreconnectClosed() const { return idlePreconnectClosed_; }
  /**
   * @brief Generate a per-boot signing key and opaque after Wi-Fi entropy is enabled.
   * 启用 Wi-Fi 熵源后生成本次启动签名密钥与 opaque。
   */
  void initializeAuthentication() {
    esp_fill_random(secret_, sizeof(secret_));
    _sopaque = _getRandomHexString();
    _srealm = "Lamp Bridge";
  }
  /**
   * @brief Validate signed, path-bound, unexpired Digest using the expected credentials.
   * 使用预期账号密码校验签名、路径与期限绑定的 Digest。
   * @param username Non-null expected username. / 非空的预期账号。
   * @param password Non-null NUL-terminated password. / 非空且 NUL 结尾的密码。
   * @return True on proof success; Cookies are not checked here. / 证明成功时 true；此处不验证 Cookie。
   */
  bool authenticateAccess(const char* username, const char* password) {
    stale_ = false;
    String authorization = header("Authorization");
    if (!authorization.startsWith("Digest ")) return false;
    const String nonce = _extractParam(authorization, "nonce=\"", '"');
    const String digestUri = _extractParam(authorization, "uri=\"", '"');
    // Bind the Digest URI to the actual canonical path to reject cross-resource header reuse.
    // API 和静态资源仅使用无查询参数的规范路径，禁止跨资源重放认证头。
    if (digestUri != uri() || nonce.length() != 72) return false;
    for (std::size_t i = 0; i < nonce.length(); ++i) {
      const char c = nonce[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    const String payload = nonce.substring(0, 40);
    const String expected = sign(payload);
    if (expected.length() != 32 || !expected.equalsConstantTime(nonce.substring(40))) return false;
    // Restore the verified nonce before framework Digest validation; other clients cannot replace it.
    // 已验签后再供旧版框架计算 Digest；其他客户端的挑战不会覆盖验证依据。
    _snonce = nonce;
    if (!WebServer::authenticate(username, password)) return false;
    const uint32_t issued =
        static_cast<uint32_t>(std::strtoul(payload.substring(0, 8).c_str(), nullptr, 16));
    stale_ = web_policy::nonceExpired(issued, millis());
    return !stale_;
  }
  /**
   * @brief Create a signed nonce; empty string means HMAC failure.
   * 创建签名 nonce；空字符串表示 HMAC 失败。
   */
  String createNonce() {
    char timestamp[9];
    std::snprintf(timestamp, sizeof(timestamp), "%08lx", static_cast<unsigned long>(millis()));
    const String payload = String(timestamp) + _getRandomHexString();
    const String signature = sign(payload);
    return signature.length() == 32 ? payload + signature : String("");
  }
  /**
   * @brief Return the current boot's opaque Digest value.
   * 返回本次启动的 opaque Digest 值。
   */
  String opaque() const { return _sopaque; }
  /**
   * @brief Require the exact issued nonce before credential/path/expiry validation.
   * 先要求精确签发 nonce，再校验账号、路径与期限。
   * @param user Non-null expected username. / 非空的预期账号。
   * @param password Non-null NUL-terminated password. / 非空且 NUL 结尾的密码。
   * @param nonce Nonce for the consumed form-login challenge. / 已消耗的表单登录挑战 nonce。
   * @return True only when all Digest checks pass. / 仅全部 Digest 检查成功时 true。
   */
  bool authenticateForNonce(const char* user, const char* password, const String& nonce) {
    String auth = header("Authorization");
    return _extractParam(auth, "nonce=\"", '"') == nonce && authenticateAccess(user, password);
  }
  /**
   * @brief Send a fresh 401 Digest challenge, or 503 on signing failure.
   * 发送新的 401 Digest 挑战；签名失败时 503。
   */
  void challenge() {
    char timestamp[9];
    std::snprintf(timestamp, sizeof(timestamp), "%08lx", static_cast<unsigned long>(millis()));
    const String payload = String(timestamp) + _getRandomHexString();
    const String signature = sign(payload);
    if (signature.length() != 32) {
      send(503, "application/json", "{\"ok\":false,\"message\":\"认证初始化失败\"}");
      return;
    }
    const String nonce = payload + signature;
    String value = "Digest realm=\"" + _srealm + "\", qop=\"auth\", algorithm=MD5, nonce=\"" +
                   nonce + "\", opaque=\"" + _sopaque + "\"";
    // For a correct password with an expired nonce, report stale=true per RFC 7616.
    // 密码正确但挑战过期时，RFC 7616 要求 stale=true，浏览器可自动换挑战而不再询问密码。
    if (stale_) value += ", stale=true";
    sendHeader("WWW-Authenticate", value);
    send(401, "application/json; charset=utf-8", "{\"ok\":false,\"message\":\"需要设备访问密码\"}");
  }

 private:
  String sign(const String& payload) const {
    // Epoch binding rejects an old proof after the 32-bit timestamp wraps.
    // 绑定计时周期，拒绝 32 位时间戳回绕后再次使用旧证明。
    const auto epoch =
        web_policy::authenticationEpoch(static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL);
    const String signingInput = String(epoch) + ":" + payload;
    unsigned char hash[32]{};
    const auto* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info || mbedtls_md_hmac(info, secret_, sizeof(secret_),
                                 reinterpret_cast<const unsigned char*>(signingInput.c_str()),
                                 signingInput.length(), hash) != 0)
      return "";
    char hex[33];
    for (std::size_t i = 0; i < 16; ++i) std::snprintf(hex + 2 * i, 3, "%02x", hash[i]);
    return String(hex);
  }
  unsigned char secret_[32]{};
  bool stale_ = false;
  uint32_t idlePreconnectClosed_ = 0;
};
