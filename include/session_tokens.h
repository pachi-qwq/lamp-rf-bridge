// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <esp_timer.h>
#include <mbedtls/sha256.h>
#include "web_policy.h"

// Unremembered sessions stay in RAM, with an eight-hour configured lifetime; reboot invalidates them.
// 未选择信任时仅保留内存会话；重启或八小时到期后失效，不写 NVS。
/**
 * @brief Hash-only eight-entry RAM session ring; no Flash writes.
 * 只存哈希的八项 RAM 会话环，不写 Flash。
 */
class SessionTokens {
 public:
  /**
   * @brief Match an active RAM session within its configured eight-hour lifetime.
   * 匹配配置八小时有效期内的活跃 RAM 会话。
   * @param cookie Existing Cookie header or null; malformed values do not match. / 原 Cookie 头或空指针；非法值不匹配。
   */
  bool containsCookie(const char* cookie) const { return find(cookie) >= 0; }
  /**
   * @brief Hash a valid token and replace the oldest slot; does not persist.
   * 哈希合法凭证并替换最旧槽位；不持久化。
   * @param token 64 lowercase hexadecimal characters, NUL-terminated. / 64 位小写十六进制，NUL 结尾。
   * @return False for invalid input or SHA-256 failure. / 输入非法或 SHA-256 失败时 false。
   */
  bool add(const char* token) {
    if (!web_policy::validTrustToken(token)) return false;
    Entry next{};
    if (mbedtls_sha256_ret(reinterpret_cast<const unsigned char*>(token), 64, next.hash, 0) != 0)
      return false;
    next.issued = static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL;
    next.active = true;
    entries_[next_++ % 8] = next;
    return true;
  }
  /**
   * @brief Deactivate a matching session; unknown/malformed input has no effect.
   * 停用匹配会话；未知或非法输入无效果。
   * @param cookie Existing Cookie header or null; malformed values do not match. / 原 Cookie 头或空指针；非法值不匹配。
   */
  void forgetCookie(const char* cookie) {
    const int index = find(cookie);
    if (index >= 0) entries_[index].active = false;
  }

 private:
  struct Entry {
    uint8_t hash[32]{};
    uint64_t issued = 0;
    bool active = false;
  };
  Entry entries_[8]{};
  std::size_t next_ = 0;
  int find(const char* cookie) const {
    char token[65]{};
    uint8_t hash[32]{};
    if (!web_policy::readTrustCookie(cookie, token) ||
        mbedtls_sha256_ret(reinterpret_cast<const unsigned char*>(token), 64, hash, 0) != 0)
      return -1;
    int result = -1;
    for (std::size_t slot = 0; slot < 8; ++slot) {
      uint8_t difference = 0;
      for (std::size_t i = 0; i < sizeof(hash); ++i) difference |= entries_[slot].hash[i] ^ hash[i];
      if (entries_[slot].active &&
          web_policy::sessionActive(entries_[slot].issued,
                                    static_cast<uint64_t>(esp_timer_get_time()) / 1000ULL) &&
          difference == 0)
        result = static_cast<int>(slot);
    }
    return result;
  }
};
