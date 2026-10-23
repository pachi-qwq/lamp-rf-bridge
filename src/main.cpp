// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <esp_system.h>

#include "cc1101_spi.h"
#include "config.h"

namespace {
cc1101::SpiDevice radio;
cc1101::StabilityTracker stability(config::kStableSampleCount);
bool initialized = false;
uint32_t lastSampleMs = 0;
uint32_t lastRetryMs = 0;
uint32_t failures = 0;

void printBusError(cc1101::BusError error) {
  const char* reason = "未初始化";
  if (error == cc1101::BusError::ReadyTimeout) {
    reason = "等待 SO/CHIP_RDYn 拉低超时";
  } else if (error == cc1101::BusError::ResetTimeout) {
    reason = "SRES 后等待 SO 拉低超时";
  }
  Serial.printf("[ERROR] %s；命令=0x%02X，CSn=%d，MISO=%d。\n", reason, radio.lastCommand(),
                digitalRead(config::pins::kCsn), digitalRead(config::pins::kMiso));
  Serial.println("当前排查：先断开 USB，再确认黑线为 CC1101 第7脚 SO -> GPIO19。");
}

const char* identityReason(cc1101::IdentityError error) {
  switch (error) {
    case cc1101::IdentityError::AllZero:
      return "ID 全为 0，不能通过：可能 SO 短路到 GND、模块无响应或时序错误";
    case cc1101::IdentityError::AllOnes:
      return "ID 全为 0xFF，不能通过：可能 SO 悬空、模块未选中或未供电";
    case cc1101::IdentityError::UnexpectedPart:
      return "PARTNUM 不符合 CC1101 的 0x00，保留原值等待排查";
    case cc1101::IdentityError::UnknownVersion:
      return "VERSION 不在已确认的 0x04/0x14 范围，需核对芯片版本";
    case cc1101::IdentityError::NotReady:
      return "状态字节 CHIP_RDYn=1，电源/晶振尚未就绪";
    case cc1101::IdentityError::NotIdle:
      return "复位后未保持 IDLE，需核查 SPI 命令和返回状态";
    default:
      return "通过";
  }
}

void initializeRadio() {
  lastRetryMs = millis();
  stability.clear();
  const auto error = radio.begin();
  initialized = error == cc1101::BusError::None;
  if (!initialized) {
    ++failures;
    printBusError(error);
    Serial.printf("[RETRY] %lu ms 后重试初始化。\n",
                  static_cast<unsigned long>(config::kRetryIntervalMs));
  } else {
    Serial.println("[INIT] 复位握手完成，开始校验真实寄存器读数。");
    lastSampleMs = millis() - config::kSampleIntervalMs;
  }
}
}  // namespace

void setup() {
  Serial.begin(config::kSerialBaud);
  delay(800);
  Serial.println("\n=== 落地灯 RF 网关 / Phase 1 / CC1101 SPI 检测 ===");
  Serial.printf("ESP32: %s rev=%u，CPU=%u MHz，Flash=%lu bytes，复位原因=%d\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getCpuFreqMHz(),
                static_cast<unsigned long>(ESP.getFlashChipSize()),
                static_cast<int>(esp_reset_reason()));
  Serial.printf("接线: CSn=%u SCK=%u MOSI=%u MISO=%u GDO0=%u GDO2=%u\n", config::pins::kCsn,
                config::pins::kSck, config::pins::kMosi, config::pins::kMiso, config::pins::kGdo0,
                config::pins::kGdo2);
  Serial.printf("SPI: %lu Hz，模式0，MSB first；串口: %lu baud\n",
                static_cast<unsigned long>(config::kSpiClockHz),
                static_cast<unsigned long>(config::kSerialBaud));
  Serial.println("检测命令: PARTNUM=0xF0 VERSION=0xF1 MARCSTATE=0xF5。");
  Serial.println("验收条件: 连续10次 ID 一致、PARTNUM=0x00、VERSION=0x04/0x14、状态为 IDLE。");
  initializeRadio();
}

void loop() {
  const uint32_t now = millis();
  if (!initialized) {
    if (static_cast<uint32_t>(now - lastRetryMs) >= config::kRetryIntervalMs) {
      initializeRadio();
    }
    delay(1);
    return;
  }
  if (static_cast<uint32_t>(now - lastSampleMs) < config::kSampleIntervalMs) {
    delay(1);
    return;
  }
  lastSampleMs = now;
  cc1101::Snapshot sample;
  const auto busError = radio.readSnapshot(sample);
  if (busError != cc1101::BusError::None) {
    ++failures;
    stability.clear();
    printBusError(busError);
    initialized = false;
    lastRetryMs = millis();
    return;
  }

  const bool wasPassed = stability.passed();
  const auto error = cc1101::validate(sample);
  stability.observe(sample);
  Serial.printf(
      "[%s] PARTNUM=0x%02X VERSION=0x%02X MARCSTATE=0x%02X "
      "STATUS=0x%02X/0x%02X/0x%02X 连续=%lu/%lu 失败=%lu\n",
      error == cc1101::IdentityError::None ? "OK" : "FAIL", sample.partnum, sample.version,
      sample.marcstate, sample.partStatus, sample.versionStatus, sample.marcStatus,
      static_cast<unsigned long>(stability.count()),
      static_cast<unsigned long>(config::kStableSampleCount),
      static_cast<unsigned long>(failures + (error != cc1101::IdentityError::None)));
  if (error != cc1101::IdentityError::None) {
    ++failures;
    Serial.printf("[DIAG] %s。\n", identityReason(error));
  } else if (!wasPassed && stability.passed()) {
    Serial.println("[PASS] 已连续10次读到一致且符合规格的芯片信息；当前 SPI 稳定性检测通过。");
  }
}
