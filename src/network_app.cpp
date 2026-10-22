// SPDX-License-Identifier: MIT
#include "network_app.h"
#include <Arduino.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "lamp_web_server.h"
#include <WiFi.h>
#include <esp_system.h>
#include "web_assets.h"
#include "web_config.h"
#include "web_policy.h"
#include "trusted_devices.h"
#include "session_tokens.h"

namespace network_app {
namespace {
LampWebServer server(web_config::kPort);
Preferences preferences;
TrustedDevices trustedDevices;
SessionTokens sessions;
struct LoginChallenge {
  String nonce, token;
  uint32_t issued = 0;
};
LoginChallenge loginChallenges[4];
std::size_t loginChallengeNext = 0;
lamp::Controller* lamp = nullptr;
struct Credentials {
  char ssid[33]{};
  char password[64]{};
} credentials;
String devicePassword, csrf, hostname, apName;
bool apActive = false, hadConnection = false, initialized = false, mdnsReady = false;
uint32_t reconnectMs = 0, disconnectedMs = 0, connectedMs = 0;
struct CachedRequest {
  String id, button, response;
  int code = 0;
};
CachedRequest history[web_config::kRequestHistory];
std::size_t historyNext = 0;

String randomHex(std::size_t bytes) {
  String output;
  output.reserve(bytes * 2);
  for (std::size_t i = 0; i < bytes; ++i) {
    char hex[3];
    std::snprintf(hex, sizeof(hex), "%02x", static_cast<unsigned>(esp_random() & 0xFF));
    output += hex;
  }
  return output;
}

String quote(const String& input) {
  String output = "\"";
  output.reserve(input.length() + 8);
  for (std::size_t i = 0; i < input.length(); ++i) {
    const uint8_t c = input[i];
    if (c == '"' || c == '\\') {
      output += '\\';
      output += static_cast<char>(c);
    } else if (c < 32) {
      char escaped[7];
      std::snprintf(escaped, sizeof(escaped), "\\u%04x", c);
      output += escaped;
    } else
      output += static_cast<char>(c);
  }
  return output + "\"";
}

void headers(const char* cacheControl = "no-store") {
  server.sendHeader("Cache-Control", cacheControl);
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.sendHeader("X-Frame-Options", "DENY");
  server.sendHeader("Referrer-Policy", "no-referrer");
  server.sendHeader("Content-Security-Policy", web_assets::kCsp);
}

void json(int code, const String& content) {
  headers();
  server.send(code, "application/json; charset=utf-8", content);
}

void fail(int code, const char* message) {
  json(code, "{\"ok\":false,\"message\":" + quote(message) + "}");
}

bool allowedHost() {
  const String host = server.hostHeader();
  const String lan = WiFi.localIP().toString();
  const String ap = WiFi.softAPIP().toString();
  const String mdns = hostname + ".local";
  return host == lan || host == lan + ":80" || (apActive && (host == ap || host == ap + ":80")) ||
         host == mdns || host == mdns + ":80";
}

bool hasAccess() {
  const String cookie = server.header("Cookie");
  return trustedDevices.containsCookie(cookie.c_str()) || sessions.containsCookie(cookie.c_str()) ||
         server.authenticateAccess(web_config::kUser, devicePassword.c_str());
}

bool authorize(bool mutation = false) {
  if (!allowedHost()) {
    fail(403, "访问地址不匹配");
    return false;
  }
  if (!hasAccess()) {
    if (server.header("X-Lamp-Web") == "1") {
      fail(401, "登录已失效，请重新登录");
      return false;
    }
    headers();
    server.challenge();
    return false;
  }
  if (mutation) {
    const String origin = server.header("Origin");
    if ((origin.length() && origin != "http://" + server.hostHeader()) ||
        server.header("X-Lamp-CSRF") != csrf) {
      fail(403, "请求校验失败，请刷新页面");
      return false;
    }
    if (!server.header("Content-Type").startsWith("application/x-www-form-urlencoded")) {
      fail(415, "请求格式不支持");
      return false;
    }
  }
  return true;
}

bool canConfigure() {
  return apActive && server.client().localIP() == WiFi.softAPIP();
}

void status() {
  if (!authorize()) return;
  lamp->healthy();
  const bool connected = WiFi.status() == WL_CONNECTED;
  String body =
      "{\"authenticated\":true,\"phase\":4,\"csrf\":" + quote(csrf) +
      ",\"auth\":{\"trusted_device\":" +
      (trustedDevices.containsCookie(server.header("Cookie").c_str()) ? "true" : "false") + "}" +
      ",\"http\":{\"idle_preconnect_closed\":" + String(server.idlePreconnectClosed()) + "}" +
      ",\"rf\":{\"ready\":" + (lamp->ready() ? "true" : "false") +
      ",\"marcstate\":" + String(lamp->marcstate()) + ",\"bus_error\":" + String(lamp->busError()) +
      "},\"network\":{\"connected\":" + (connected ? "true" : "false") +
      ",\"configured\":" + (credentials.ssid[0] ? "true" : "false") +
      ",\"ap_active\":" + (apActive ? "true" : "false") +
      ",\"can_configure\":" + (canConfigure() ? "true" : "false") +
      ",\"ip\":" + quote(connected ? WiFi.localIP().toString() : String("")) +
      ",\"ssid\":" + quote(connected ? WiFi.SSID() : String("")) +
      "},\"attempts\":" + String(lamp->attempts()) +
      ",\"cooldown_ms\":" + String(lamp->cooldownMs()) +
      ",\"last_button\":" + quote(lamp->lastButton()) + ",\"uptime_s\":" + String(millis() / 1000) +
      "}";
  json(200, body);
}

void command() {
  if (!authorize(true)) return;
  const String button = server.arg("button"), id = server.arg("request_id");
  if (server.args() != 2 || button.length() != std::strlen(button.c_str()) ||
      id.length() != std::strlen(id.c_str()) || !replay::findFrame(button.c_str()) ||
      !web_policy::validRequestId(id.c_str())) {
    fail(400, "无效按键或请求编号");
    return;
  }
  for (const auto& cached : history) {
    if (cached.id == id) {
      if (cached.button != button)
        fail(409, "请求编号已被其他按键使用");
      else
        json(cached.code, cached.response);
      return;
    }
  }
  // HTTP and RF run serially in the Arduino task; Wi-Fi callbacks never access SPI.
  // HTTP 处理与射频发送在 Arduino 同一任务中串行执行，Wi-Fi 回调不访问 SPI。
  const auto result = lamp->send(button.c_str());
  int code = 200;
  String message;
  switch (result) {
    case lamp::SendCode::Sent:
      message = "已发送";
      break;
    case lamp::SendCode::Cooldown:
      code = 429;
      message = "操作太快，请稍等";
      break;
    case lamp::SendCode::UnknownButton:
      code = 400;
      message = "无效按键";
      break;
    case lamp::SendCode::NotReady:
      code = 503;
      message = "射频模块未就绪";
      break;
    case lamp::SendCode::HardwareError:
      code = 503;
      message = "射频发送异常，请检查设备";
      break;
  }
  const String response =
      "{\"ok\":" + String(code == 200 ? "true" : "false") + ",\"message\":" + quote(message) +
      ",\"button\":" + quote(button) + ",\"attempt\":" + String(lamp->attempts()) +
      ",\"cooldown_ms\":" + String(lamp->cooldownMs()) + ",\"radio_elapsed_us\":" +
      String(result == lamp::SendCode::Sent ? lamp->lastResult().elapsedUs : 0) +
      ",\"frames\":" + String(replay::findFrame(button.c_str())->repeats) +
      ",\"lamp_state_confirmed\":false}";
  // Cache the last eight request results; the UI never retries automatically to avoid duplicate toggles.
  // 最近 8 个请求编号复用结果；前端不自动重试，避免网络重传重复切换。
  auto& cached = history[historyNext++ % web_config::kRequestHistory];
  cached = {id, button, response, code};
  json(code, response);
}

void loginChallenge() {
  if (!allowedHost()) {
    fail(403, "访问地址不匹配");
    return;
  }
  auto& challenge = loginChallenges[loginChallengeNext++ % 4];
  challenge.nonce = server.createNonce();
  challenge.token = randomHex(16);
  challenge.issued = millis();
  if (!challenge.nonce.length()) {
    fail(503, "认证初始化失败");
    return;
  }
  json(200, "{\"realm\":\"Lamp Bridge\",\"nonce\":" + quote(challenge.nonce) + ",\"opaque\":" +
                quote(server.opaque()) + ",\"csrf\":" + quote(challenge.token) + "}");
}

void login() {
  if (!allowedHost()) {
    fail(403, "访问地址不匹配");
    return;
  }
  const String origin = server.header("Origin");
  if ((origin.length() && origin != "http://" + server.hostHeader()) ||
      !server.header("Content-Type").startsWith("application/x-www-form-urlencoded") ||
      server.args() != 1 || (server.arg("remember") != "0" && server.arg("remember") != "1")) {
    fail(403, "登录请求无效");
    return;
  }
  LoginChallenge* match = nullptr;
  for (auto& challenge : loginChallenges) {
    if (challenge.token.length() && challenge.token == server.header("X-Lamp-CSRF") &&
        static_cast<uint32_t>(millis() - challenge.issued) <= 120000U) {
      match = &challenge;
      break;
    }
  }
  if (!match) {
    fail(403, "登录信息已过期，请重新登录");
    return;
  }
  const String nonce = match->nonce;
  *match =
      {};  // Consume each challenge once, even for a wrong password; a proof cannot mint another session. / 每次挑战只用一次，包括错误密码，不允许重复登录证明换取新会话。
  if (!server.authenticateForNonce(web_config::kUser, devicePassword.c_str(), nonce)) {
    fail(403, "账号或密码不正确");
    return;
  }
  const bool remember = server.arg("remember") == "1";
  const String token = randomHex(32);
  if (!(remember ? trustedDevices.replaceCookie(server.header("Cookie").c_str(), token.c_str())
                 : sessions.add(token.c_str())) ||
      (!remember && !trustedDevices.forgetCookie(server.header("Cookie").c_str()))) {
    fail(500, "无法保存登录信息");
    return;
  }
  sessions.forgetCookie(server.header("Cookie").c_str());
  server.sendHeader("Set-Cookie", "lamp_device=" + token + "; Path=/; HttpOnly; SameSite=Strict" +
                                      (remember ? "; Max-Age=31536000" : ""));
  json(200, "{\"ok\":true,\"trusted_device\":" + String(remember ? "true" : "false") + "}");
}

void sessionStatus() {
  if (!allowedHost()) {
    fail(403, "访问地址不匹配");
    return;
  }
  if (!hasAccess()) {
    json(200, "{\"authenticated\":false}");
    return;
  }
  status();
}

void trustDevice() {
  if (!authorize(true)) return;
  if (server.args() != 0) {
    fail(400, "请求参数不支持");
    return;
  }
  if (trustedDevices.containsCookie(server.header("Cookie").c_str())) {
    json(200, "{\"ok\":true,\"trusted_device\":true}");
    return;
  }
  const String token = randomHex(32);
  if (!trustedDevices.add(token.c_str())) {
    fail(500, "无法保存设备信任");
    return;
  }
  // LAN HTTP uses HttpOnly Cookies together with SameSite and CSRF checks.
  // 当前使用局域网 HTTP；HttpOnly 阻止脚本读取，SameSite 与 CSRF 校验同时保留。
  // Omit Domain to bind the token to this host; browser storage lasts up to a year and is revocable.
  // 不设置 Domain，凭证只属于当前地址；浏览器最多保留一年，可随时撤销。
  server.sendHeader("Set-Cookie", "lamp_device=" + token +
                                      "; Path=/; Max-Age=31536000; HttpOnly; SameSite=Strict");
  json(200, "{\"ok\":true,\"trusted_device\":true}");
}

void forgetDevice() {
  if (!authorize(true)) return;
  if (server.args() != 0) {
    fail(400, "请求参数不支持");
    return;
  }
  if (!trustedDevices.forgetCookie(server.header("Cookie").c_str())) {
    fail(500, "无法撤销设备信任");
    return;
  }
  sessions.forgetCookie(server.header("Cookie").c_str());
  const String session = randomHex(32);
  if (!sessions.add(session.c_str())) {
    fail(500, "无法更新当前登录");
    return;
  }
  server.sendHeader("Set-Cookie", "lamp_device=" + session + "; Path=/; HttpOnly; SameSite=Strict");
  json(200, "{\"ok\":true,\"trusted_device\":false}");
}

void startConnection() {
  reconnectMs = millis();
  if (credentials.ssid[0]) WiFi.begin(credentials.ssid, credentials.password);
}

void configureWifi() {
  if (!authorize(true)) return;
  if (!canConfigure()) {
    fail(403, "请连接设备热点后配网");
    return;
  }
  const String ssid = server.arg("ssid"), password = server.arg("password");
  if (server.args() != 2 || ssid.length() != std::strlen(ssid.c_str()) ||
      password.length() != std::strlen(password.c_str()) ||
      !web_policy::validWifi(ssid.c_str(), password.c_str())) {
    fail(400, "请检查 Wi-Fi 名称与密码长度");
    return;
  }
  Credentials next{};
  ssid.toCharArray(next.ssid, sizeof(next.ssid));
  password.toCharArray(next.password, sizeof(next.password));
  // Persist SSID and password in one NVS record to avoid partial updates.
  // 整组信息一次写入 NVS，避免只更新名称或密码的一半。
  if (preferences.putBytes("network", &next, sizeof(next)) != sizeof(next)) {
    fail(500, "保存失败，请重试配网");
    return;
  }
  credentials = next;
  WiFi.disconnect(false, false);
  hadConnection = false;
  disconnectedMs = millis();
  connectedMs = 0;
  json(200, "{\"ok\":true,\"message\":\"已保存，正在连接\"}");
  startConnection();
  Serial.println("{\"type\":\"wifi_config_saved\"}");
}
}  // namespace

void printAccess() {
  if (!initialized) return;
  // Output these secrets only over physical USB, never through the public API or repository.
  // 此信息只通过物理 USB 输出，不写进公开 API 或版本库。
  Serial.printf(
      "{\"type\":\"access\",\"hostname\":\"%s.local\",\"ap_ssid\":\"%s\","
      "\"ap_password\":\"%s\",\"username\":\"%s\",\"access_password\":\"%s\"}\n",
      hostname.c_str(), apName.c_str(), devicePassword.c_str(), web_config::kUser,
      devicePassword.c_str());
}

void enableSetup() {
  if (apActive || !initialized) return;
  WiFi.mode(WIFI_AP_STA);
  apActive = WiFi.softAP(apName.c_str(), devicePassword.c_str(), web_config::kApChannel, false,
                         web_config::kMaxApClients);
  if (apActive && WiFi.status() == WL_CONNECTED) connectedMs = millis();
  Serial.printf("{\"type\":\"setup_ap\",\"active\":%s,\"ip\":\"%s\"}\n",
                apActive ? "true" : "false", WiFi.softAPIP().toString().c_str());
}

bool begin(lamp::Controller& controller) {
  lamp = &controller;
  if (!preferences.begin("lamp", false)) return false;
  if (!trustedDevices.begin(preferences)) return false;
  const auto size = preferences.getBytesLength("network");
  if (size == sizeof(credentials))
    preferences.getBytes("network", &credentials, sizeof(credentials));
  credentials.ssid[32] = 0;
  credentials.password[63] = 0;
  if (!web_policy::validWifi(credentials.ssid, credentials.password)) credentials = {};
  // Enable the Wi-Fi entropy source before generating the access password and CSRF token.
  // 先启用 Wi-Fi 的物理熵源，再生成访问密码和 CSRF 随机值。
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  devicePassword = preferences.getString("access", "");
  if (devicePassword.length() != 16) {
    devicePassword = randomHex(8);
    if (preferences.putString("access", devicePassword) != 16) return false;
  }
  // Regenerate the CSRF token each boot; it never stores or infers lamp state.
  // 会话校验值每次启动重新生成，不用于推断或保存灯具状态。
  csrf = randomHex(16);
  server.initializeAuthentication();
  const uint64_t mac = ESP.getEfuseMac();
  char suffix[7];
  std::snprintf(suffix, sizeof(suffix), "%06lx",
                static_cast<unsigned long>((mac >> 24) & 0xFFFFFF));
  hostname = "lamp-" + String(suffix);
  apName = "Lamp-" + String(suffix);
  WiFi.setHostname(hostname.c_str());
  WiFi.setAutoReconnect(true);
  initialized = true;
  disconnectedMs = reconnectMs = millis();
  if (credentials.ssid[0])
    startConnection();
  else {
    enableSetup();
    printAccess();
  }
  const char* keys[] = {"Origin", "X-Lamp-CSRF", "Content-Type", "Cookie", "X-Lamp-Web"};
  server.collectHeaders(keys, 5);
  server.on("/", HTTP_GET, [] {
    if (!allowedHost()) {
      fail(403, "访问地址不匹配");
      return;
    }
    headers();
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/html; charset=utf-8",
                  reinterpret_cast<const char*>(web_assets::kIndex), sizeof(web_assets::kIndex));
  });
  server.on("/login", HTTP_GET, [] {
    if (!allowedHost()) {
      fail(403, "访问地址不匹配");
      return;
    }
    headers();
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/html; charset=utf-8",
                  reinterpret_cast<const char*>(web_assets::kIndex), sizeof(web_assets::kIndex));
  });
  server.on("/style.css", HTTP_GET, [] {
    if (!allowedHost()) {
      fail(403, "访问地址不匹配");
      return;
    }
    headers();
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/css; charset=utf-8", reinterpret_cast<const char*>(web_assets::kStyle),
                  sizeof(web_assets::kStyle));
  });
  server.on("/app.js", HTTP_GET, [] {
    if (!allowedHost()) {
      fail(403, "访问地址不匹配");
      return;
    }
    headers();
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "application/javascript; charset=utf-8",
                  reinterpret_cast<const char*>(web_assets::kScript), sizeof(web_assets::kScript));
  });
  // Installation metadata/icons grant no control access; allow iOS icon fetchers to read them.
  // 安装元数据和图标不含控制权限，允许 iOS 主屏幕图标抓取器直接读取。
  server.on("/manifest.webmanifest", HTTP_GET, [] {
    headers();
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "application/manifest+json; charset=utf-8",
                  reinterpret_cast<const char*>(web_assets::kManifest),
                  sizeof(web_assets::kManifest));
  });
  server.on("/icon.svg", HTTP_GET, [] {
    headers();
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "image/svg+xml", reinterpret_cast<const char*>(web_assets::kIcon),
                  sizeof(web_assets::kIcon));
  });
  server.on("/apple-touch-icon.png", HTTP_GET, [] {
    headers();
    server.send_P(200, "image/png", reinterpret_cast<const char*>(web_assets::kAppleIcon),
                  sizeof(web_assets::kAppleIcon));
  });
  // A versioned URL lets iOS cache the launch image instead of downloading it at every launch.
  // 固定版本 URL 便于 iOS 缓存启动画面，避免启动阶段等待网络下载背景。
  server.on("/launch-1206x2622-v1.png", HTTP_GET, [] {
    headers("public, max-age=31536000, immutable");
    server.send_P(200, "image/png", reinterpret_cast<const char*>(web_assets::kLaunch),
                  sizeof(web_assets::kLaunch));
  });
  server.on("/api/session", HTTP_GET, sessionStatus);
  server.on("/api/login/challenge", HTTP_GET, loginChallenge);
  server.on("/api/session/login", HTTP_POST, login);
  server.on("/api/status", HTTP_GET, status);
  server.on("/api/command", HTTP_POST, command);
  server.on("/api/wifi", HTTP_POST, configureWifi);
  server.on("/api/session/trust", HTTP_POST, trustDevice);
  server.on("/api/session/forget", HTTP_POST, forgetDevice);
  server.onNotFound([] {
    if (authorize()) fail(404, "接口不存在");
  });
  server.begin();
  Serial.println("{\"type\":\"http_ready\",\"port\":80,\"automatic_transmit\":false}");
  return true;
}

void loop() {
  if (!initialized) return;
  server.handleClient();
  const uint32_t now = millis();
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !hadConnection) {
    connectedMs = now;
    Serial.printf("{\"type\":\"wifi_connected\",\"ip\":\"%s\"}\n",
                  WiFi.localIP().toString().c_str());
    if (!mdnsReady) {
      mdnsReady = MDNS.begin(hostname.c_str());
      if (mdnsReady) MDNS.addService("http", "tcp", web_config::kPort);
    }
  } else if (!connected && hadConnection) {
    disconnectedMs = now;
    if (mdnsReady) {
      MDNS.end();
      mdnsReady = false;
    }
    Serial.println("{\"type\":\"wifi_disconnected\",\"reconnecting\":true}");
  }
  hadConnection = connected;
  if (!connected && credentials.ssid[0]) {
    if (static_cast<uint32_t>(now - reconnectMs) >= web_config::kReconnectMs) startConnection();
    if (!apActive && static_cast<uint32_t>(now - disconnectedMs) >= web_config::kFallbackApMs)
      enableSetup();
  }
  if (connected && apActive && static_cast<uint32_t>(now - connectedMs) >= web_config::kApGraceMs) {
    WiFi.softAPdisconnect(false);
    apActive = false;
    Serial.println("{\"type\":\"setup_ap_closed\"}");
  }
}
}  // namespace network_app
