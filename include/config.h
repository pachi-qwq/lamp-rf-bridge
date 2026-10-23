// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace config {
// Keep the verified wiring. Power CC1101 from 3V3, never 5V/VIN.
// 与用户已核对的接线保持一致。CC1101 只能接 3V3，不能接 5V/VIN。
namespace pins {
inline constexpr uint8_t kCsn = 5;    // Blue wire, CC1101 pin 4 / 蓝线，CC1101 第 4 脚
inline constexpr uint8_t kSck = 18;   // Green wire, pin 5 / 绿线，第 5 脚
inline constexpr uint8_t kMosi = 23;  // Purple wire, pin 6 (SI) / 紫线，第 6 脚（SI）
inline constexpr uint8_t kMiso =
    19;  // Black wire, pin 7 (SO/CHIP_RDYn) / 黑线，第 7 脚（SO/CHIP_RDYn）
inline constexpr uint8_t kGdo0 = 26;  // Gray wire, pin 3 / 灰线，第 3 脚
inline constexpr uint8_t kGdo2 = 27;  // Orange wire, pin 8 / 橙线，第 8 脚
}  // namespace pins

inline constexpr uint32_t kSerialBaud = 115200;
inline constexpr uint32_t kSpiClockHz = 1000000;
inline constexpr uint32_t kReadyTimeoutUs = 10000;
inline constexpr uint32_t kResetPulseUs = 10;
inline constexpr uint32_t kResetHighUs = 50;  // TI requires at least 40 us / TI 要求至少 40 us
inline constexpr uint32_t kSampleIntervalMs = 1000;
inline constexpr uint32_t kRetryIntervalMs = 5000;
inline constexpr uint32_t kStableSampleCount = 10;
}  // namespace config
