// SPDX-License-Identifier: MIT
#pragma once
#include <Preferences.h>
#include <mbedtls/sha256.h>
#include "web_policy.h"

// Store only SHA-256 hashes of 256-bit tokens, not Cookie plaintext; trust at most four clients.
// 仅保存 256 位随机凭证的 SHA-256，不保存 Cookie 明文；最多信任四台客户端。
// NVS survives reboot; revocation removes the hash immediately. Reads do not write Flash.
// NVS 跨重启保留，取消信任会立即删除对应哈希。普通读取不写 Flash。
/**
 * @brief Persist up to four SHA-256 token hashes; never store Cookie plaintext.
 * 持久化最多四个 SHA-256 凭证哈希；不保存 Cookie 明文。
 */
class TrustedDevices {
 public:
  /**
   * @brief Load a valid version-1 record, otherwise start empty; caller opens Preferences.
   * 加载合法版本 1 记录，否则为空；调用方先打开 Preferences。
   * @param preferences Opened NVS Preferences instance that outlives this object. / 已打开且生命周期长于本对象的 NVS Preferences。
   * @return Always true; invalid/missing records are treated as empty. / 始终 true；非法或缺失记录按空处理。
   */
  bool begin(Preferences& preferences) {
    preferences_ = &preferences;
    Records loaded{};
    if (preferences.getBytesLength("trusted") == sizeof(loaded) &&
        preferences.getBytes("trusted", &loaded, sizeof(loaded)) == sizeof(loaded) &&
        loaded.version == 1 && loaded.next < kCapacity)
      records_ = loaded;
    return true;
  }
  /**
   * @brief Match one exact Cookie against persistent hashes; reject null.
   * 将精确 Cookie 与持久哈希匹配；拒绝空指针。
   * @param cookie Existing Cookie header or null; malformed values do not match. / 原 Cookie 头或空指针；非法值不匹配。
   */
  bool containsCookie(const char* cookie) const {
    char token[65]{};
    return web_policy::readTrustCookie(cookie, token) && findToken(token) >= 0;
  }
  /**
   * @brief Store a hash in a free slot or rotate the full ring; duplicates succeed.
   * 在空槽存哈希，满时轮换；重复值视为成功。
   * @param token 64 lowercase hexadecimal characters, NUL-terminated. / 64 位小写十六进制，NUL 结尾。
   * @return False for bad token, unavailable storage or failed NVS write. / 凭证非法、存储不可用或 NVS 写入失败时 false。
   */
  bool add(const char* token) {
    uint8_t hash[32]{};
    if (!hashToken(token, hash) || !preferences_) return false;
    if (findToken(token) >= 0) return true;
    Records next = records_;
    const uint8_t empty[32]{};
    std::size_t slot = next.next;
    for (std::size_t i = 0; i < kCapacity; ++i) {
      if (std::memcmp(next.hashes[i], empty, sizeof(empty)) == 0) {
        slot = i;
        break;
      }
    }
    std::memcpy(next.hashes[slot], hash, sizeof(hash));
    next.next = static_cast<uint32_t>((slot + 1) % kCapacity);
    return persist(next);
  }
  /**
   * @brief Atomically replace a matching old hash, otherwise add a new one.
   * 原子替换匹配的旧哈希，否则新增。
   * @param cookie Existing Cookie header or null; malformed values do not match. / 原 Cookie 头或空指针；非法值不匹配。
   * @param token 64 lowercase hexadecimal characters, NUL-terminated. / 64 位小写十六进制，NUL 结尾。
   * @return False on hashing/persistence failure; RAM changes only on success. / 哈希或持久化失败时 false；仅成功后更新内存。
   */
  bool replaceCookie(const char* cookie, const char* token) {
    char previous[65]{};
    const int slot = web_policy::readTrustCookie(cookie, previous) ? findToken(previous) : -1;
    if (slot < 0) return add(token);
    uint8_t hash[32]{};
    if (!hashToken(token, hash)) return false;
    Records next = records_;
    std::memcpy(next.hashes[slot], hash, sizeof(hash));
    return persist(next);
  }
  /**
   * @brief Remove a matching hash; unknown/malformed Cookies succeed without writing.
   * 删除匹配哈希；未知或非法 Cookie 直接成功且不写入。
   * @param cookie Existing Cookie header or null; malformed values do not match. / 原 Cookie 头或空指针；非法值不匹配。
   * @return False only if removal persistence fails. / 仅删除持久化失败时 false。
   */
  bool forgetCookie(const char* cookie) {
    char token[65]{};
    if (!web_policy::readTrustCookie(cookie, token)) return true;
    const int slot = findToken(token);
    if (slot < 0) return true;
    Records next = records_;
    std::memset(next.hashes[slot], 0, sizeof(next.hashes[slot]));
    return persist(next);
  }

 private:
  static constexpr std::size_t kCapacity = 4;
  struct Records {
    uint32_t version = 1;
    uint32_t next = 0;
    uint8_t hashes[kCapacity][32]{};
  } records_;
  Preferences* preferences_ = nullptr;
  static bool hashToken(const char* token, uint8_t (&hash)[32]) {
    return web_policy::validTrustToken(token) &&
           mbedtls_sha256_ret(reinterpret_cast<const unsigned char*>(token), 64, hash, 0) == 0;
  }
  int findToken(const char* token) const {
    uint8_t hash[32]{};
    if (!hashToken(token, hash)) return -1;
    int found = -1;
    for (std::size_t slot = 0; slot < kCapacity; ++slot) {
      uint8_t difference = 0;
      for (std::size_t i = 0; i < sizeof(hash); ++i)
        difference |= hash[i] ^ records_.hashes[slot][i];
      if (difference == 0) found = static_cast<int>(slot);
    }
    return found;
  }
  bool persist(const Records& next) {
    if (!preferences_ || preferences_->putBytes("trusted", &next, sizeof(next)) != sizeof(next))
      return false;
    records_ = next;
    return true;
  }
};
