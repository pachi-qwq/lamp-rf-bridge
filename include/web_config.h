// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
namespace web_config {
inline constexpr uint16_t kPort = 80;
inline constexpr const char* kUser = "admin";
inline constexpr uint32_t kReconnectMs = 15000;
inline constexpr uint32_t kFallbackApMs = 60000;
inline constexpr uint32_t kApGraceMs = 180000;
inline constexpr uint8_t kApChannel = 6;
inline constexpr uint8_t kMaxApClients = 2;
inline constexpr uint8_t kRequestHistory = 8;
// Under contention, release a byte-free preconnection after at least 200 ms.
// 有其他连接排队时，无请求字节的预连接最多独占 HTTP 入口 200ms。
inline constexpr uint32_t kIdlePreconnectWaitMs = 200;
}  // namespace web_config
