// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>

namespace radio_config {
// Assume a 26 MHz module crystal; nominal settings worked for the retained captures and replay but are uncalibrated.
// 模块晶振按 26 MHz；默认参数已用于留存采样与重放，尚未独立校准。
inline constexpr uint32_t kCrystalHz = 26000000;
inline constexpr uint32_t kDefaultFrequencyHz = 433920000;
inline constexpr uint32_t kMinFrequencyHz = 430000000;
inline constexpr uint32_t kMaxFrequencyHz = 437000000;
inline constexpr uint8_t kFoccfg = 0x14;  // ASK/OOK requires FOC_LIMIT=0 / ASK/OOK 必须 FOC_LIMIT=0
inline constexpr uint8_t kAgcCtrl2 = 0x04;
inline constexpr uint8_t kAgcCtrl1 = 0x00;
inline constexpr uint8_t kAgcCtrl0 = 0x92;
inline constexpr uint8_t kRssiOffsetDb =
    74;  // TI typical value, not independently calibrated / TI 典型值，非实测校准值

/**
 * @brief Receive demodulator register set assuming kCrystalHz.
 * 按 kCrystalHz 计算的接收解调寄存器集合。
 */
struct Profile {
  const char* name;
  uint8_t mdmcfg4;
  uint8_t mdmcfg3;
  uint8_t fsctrl1;
  uint8_t frend1;
  uint8_t fifothr;
  uint8_t test2;
  uint8_t test1;
};

// Bandwidth settings follow TI DN022; the receiver data rate does not establish the remote's bit rate.
// 带宽相关寄存器遵循 TI DN022；数据率只影响接收解调，不能据此推断遥控器速率。
inline constexpr Profile kProfiles[] = {
    {"default", 0x87, 0x83, 0x06, 0xB6, 0x47, 0x81, 0x35},  // 203125 Hz / 4798 Baud
    {"wide", 0x47, 0x83, 0x0C, 0xB6, 0x07, 0x88, 0x31},     // 406250 Hz / 4798 Baud
    {"narrow", 0xD7, 0x83, 0x06, 0x56, 0x47, 0x81, 0x35},   // 81250 Hz / 4798 Baud
    {"fast", 0x88, 0x83, 0x06, 0xB6, 0x47, 0x81, 0x35},     // 203125 Hz / 9596 Baud
};
inline constexpr std::size_t kProfileCount = sizeof(kProfiles) / sizeof(kProfiles[0]);

/**
 * @brief Round nominal Hz to a CC1101 frequency word using kCrystalHz.
 * 使用 kCrystalHz 将名义 Hz 四舍五入为 CC1101 频率字。
 * @param hz Nominal Hz; caller checks the module frequency range. / 名义 Hz；调用方验证模块频率范围。
 */
constexpr uint32_t frequencyWord(uint32_t hz) {
  return static_cast<uint32_t>((static_cast<uint64_t>(hz) * 65536 + kCrystalHz / 2) / kCrystalHz);
}
/**
 * @brief Decode channel bandwidth in Hz from MDMCFG4 and kCrystalHz.
 * 由 MDMCFG4 与 kCrystalHz 解码接收带宽，单位 Hz。
 * @param profile Receiver register set. / 接收寄存器集合。
 */
constexpr uint32_t bandwidthHz(const Profile& profile) {
  return kCrystalHz / (8U * (4U + ((profile.mdmcfg4 >> 4) & 3U)) * (1U << (profile.mdmcfg4 >> 6)));
}
/**
 * @brief Decode configured receiver milli-baud; this is not a measured protocol bit rate.
 * 解码接收器配置速率，单位 milli-baud；不是实测协议位速率。
 * @param profile Receiver register set. / 接收寄存器集合。
 */
constexpr uint32_t dataRateMilliBaud(const Profile& profile) {
  return static_cast<uint32_t>((static_cast<uint64_t>(256U + profile.mdmcfg3) *
                                (1ULL << (profile.mdmcfg4 & 15U)) * kCrystalHz * 1000U) >>
                               28);
}
/**
 * @brief Convert raw RSSI to twice nominal dBm using the uncalibrated TI offset.
 * 使用未经校准的 TI 偏移，将 RSSI 转为名义 dBm 的两倍。
 * @param raw Raw register byte, interpreted as signed RSSI. / 原始寄存器字节，按有符号 RSSI 解读。
 */
constexpr int rssiDbmTimes2(uint8_t raw) {
  const int signedValue = raw >= 128 ? static_cast<int>(raw) - 256 : raw;
  return signedValue - 2 * kRssiOffsetDb;
}
}  // namespace radio_config
