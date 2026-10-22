// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace web_policy {
/**
 * @brief Check the RAM session lifetime against a 64-bit monotonic millisecond clock.
 * 使用 64 位单调毫秒时钟检查 RAM 会话期限。
 * @param issued Issuance uptime in milliseconds. / 签发时启动毫秒数。
 * @param now Current uptime in milliseconds. / 当前启动毫秒数。
 * @return True before eight hours, never for a future-issued timestamp. / 未满八小时且不为未来签发时间时 true。
 */
constexpr bool sessionActive(uint64_t issued, uint64_t now) {
  return now >= issued && now - issued < 28800000ULL;
}
/**
 * @brief Bind nonce signatures to the current 32-bit millis() epoch.
 * 将 nonce 签名绑定到当前 32 位 millis() 计时周期。
 * @param uptimeMs Full 64-bit monotonic uptime in milliseconds. / 完整 64 位单调启动毫秒数。
 * @return Epoch changes every 2^32 ms, preventing old signed challenges from reviving. / 每 2^32ms 改变，阻止旧签名挑战复活。
 */
constexpr uint32_t authenticationEpoch(uint64_t uptimeMs) {
  return static_cast<uint32_t>(uptimeMs >> 32);
}

/**
 * @brief Check one-hour expiry with unsigned millis() subtraction; signatures also bind the clock epoch.
 * 通过无符号 millis() 差检查一小时过期；签名另绑定计时周期。
 * @param issued Unsigned millis() timestamp of issuance. / 签发时无符号 millis() 时间戳。
 * @param now Current unsigned millis() value. / 当前无符号 millis() 值。
 * @return True strictly after 3600000 ms. / 严格超过 3600000ms 时 true。
 */
inline bool nonceExpired(uint32_t issued, uint32_t now) {
  return static_cast<uint32_t>(now - issued) > 3600000U;
}
/**
 * @brief Accept 16..48 lowercase hexadecimal characters.
 * 接受 16..48 位小写十六进制字符。
 * @param text NUL-terminated input; non-null except validTrustToken() rejects null. / NUL 结尾输入；除 validTrustToken() 拒绝空指针外须非空。
 */
inline bool validRequestId(const char* text) {
  const std::size_t size = std::strlen(text);
  if (size < 16 || size > 48) return false;
  for (std::size_t i = 0; i < size; ++i) {
    const char c = text[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}
/**
 * @brief Check control characters and byte lengths: SSID 1..32, password empty or 8..63.
 * 检查控制字符与字节长度：SSID 1..32，密码为空或 8..63。
 * @param ssid Non-null NUL-terminated SSID, 1..32 bytes. / 非空且 NUL 结尾的 SSID，1..32 字节。
 * @param password Non-null NUL-terminated password. / 非空且 NUL 结尾的密码。
 */
inline bool validWifi(const char* ssid, const char* password) {
  const auto nameSize = std::strlen(ssid), passwordSize = std::strlen(password);
  if (nameSize == 0 || nameSize > 32 || passwordSize > 63 || (passwordSize > 0 && passwordSize < 8))
    return false;
  for (std::size_t i = 0; i < nameSize; ++i)
    if (static_cast<uint8_t>(ssid[i]) < 32 || ssid[i] == 127) return false;
  for (std::size_t i = 0; i < passwordSize; ++i)
    if (static_cast<uint8_t>(password[i]) < 32 || password[i] == 127) return false;
  return true;
}

// Match Cookie names exactly and reject duplicates to prevent ambiguous authentication.
// Cookie 名精确匹配、拒绝重复，避免相似前缀或解析歧义绕过认证。
/**
 * @brief Accept exactly 64 lowercase hex characters; reject null.
 * 仅接受 64 位小写十六进制字符；拒绝空指针。
 * @param text NUL-terminated input; non-null except validTrustToken() rejects null. / NUL 结尾输入；除 validTrustToken() 拒绝空指针外须非空。
 */
inline bool validTrustToken(const char* text) {
  if (!text || std::strlen(text) != 64) return false;
  for (std::size_t i = 0; i < 64; ++i) {
    const char c = text[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}
/**
 * @brief Extract one unambiguous lamp_device Cookie; reject malformed/duplicate values.
 * 提取唯一明确的 lamp_device Cookie；拒绝非法或重复值。
 * @param header NUL-terminated Cookie header; null is rejected. / NUL 结尾 Cookie 头；拒绝空指针。
 * @param token 64 lowercase hexadecimal characters; readTrustCookie() uses a 65-byte output array. / 64 位小写十六进制；readTrustCookie() 使用 65 字节输出数组。
 * @return True for one syntactically valid token; does not authenticate it. / 一个格式合法凭证时 true；不验证身份。
 */
inline bool readTrustCookie(const char* header, char (&token)[65]) {
  token[0] = 0;
  if (!header) return false;
  constexpr char name[] = "lamp_device=";
  bool found = false;
  while (*header) {
    while (*header == ' ' || *header == '\t' || *header == ';') ++header;
    const char* end = std::strchr(header, ';');
    if (!end) end = header + std::strlen(header);
    if (static_cast<std::size_t>(end - header) >= sizeof(name) - 1 &&
        std::strncmp(header, name, sizeof(name) - 1) == 0) {
      if (found || end - header != static_cast<std::ptrdiff_t>(sizeof(name) - 1 + 64)) return false;
      std::memcpy(token, header + sizeof(name) - 1, 64);
      token[64] = 0;
      if (!validTrustToken(token)) return false;
      found = true;
    }
    header = end;
    if (*header) ++header;
  }
  return found;
}
}  // namespace web_policy
