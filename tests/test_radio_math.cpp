// SPDX-License-Identifier: MIT
// Test numeric conversions only; do not simulate SPI or real RF responses.
// 仅测试纯数值换算，不模拟 SPI 或真实射频响应。
#include <cstdio>
#include "radio_config.h"

int main() {
  int checks = 0, errors = 0;
  const auto check = [&](bool ok, const char* name) {
    ++checks;
    if (!ok) {
      ++errors;
      std::printf("FAIL: %s\n", name);
    }
  };
  using namespace radio_config;
  check(frequencyWord(433920000) == 0x10B071, "433.92MHz word");
  check(frequencyWord(433000000) == 0x10A762, "433MHz word rounding");
  check(bandwidthHz(kProfiles[0]) == 203125, "default BW");
  check(bandwidthHz(kProfiles[1]) == 406250, "wide BW");
  check(bandwidthHz(kProfiles[2]) == 81250, "narrow BW");
  check(dataRateMilliBaud(kProfiles[0]) > 4797000 && dataRateMilliBaud(kProfiles[0]) < 4799000,
        "4.8k rate");
  check(dataRateMilliBaud(kProfiles[3]) > 9594000 && dataRateMilliBaud(kProfiles[3]) < 9598000,
        "9.6k rate");
  check(rssiDbmTimes2(0) == -148, "RSSI zero");
  check(rssiDbmTimes2(127) == -21, "RSSI positive maximum");
  check(rssiDbmTimes2(128) == -276, "RSSI negative boundary");
  check(rssiDbmTimes2(255) == -149, "RSSI negative half dB");
  check((kFoccfg & 3U) == 0, "OOK FOC_LIMIT must be zero");
  std::printf("%d radio math checks, %d failures (no hardware test).\n", checks, errors);
  return errors ? 1 : 0;
}
