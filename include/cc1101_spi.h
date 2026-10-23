// SPDX-License-Identifier: MIT
#pragma once
#include <SPI.h>
#include "cc1101_identity.h"
#include "radio_config.h"

namespace cc1101 {
/**
 * @brief Bus/configuration/state result; None means success.
 * 总线、配置与状态结果；None 表示成功。
 */
enum class BusError {
  None,
  ReadyTimeout,
  ResetTimeout,
  NotInitialized,
  RegisterMismatch,
  StateTimeout,
  InvalidSettings,
  ChipNotReady
};
/**
 * @brief Raw radio state, RSSI and packet flags; not lamp state.
 * 原始模块状态、RSSI 与包标志；不是灯具状态。
 */
struct RuntimeStatus {
  uint8_t marcstate = 0;
  uint8_t rssi = 0;
  uint8_t pktstatus = 0;
};

// Provide identity checks, receive control and fixed-setting OOK transmit control.
// 提供身份检查、接收控制与固定参数的 OOK 发射控制。
/**
 * @brief Single-task CC1101 SPI driver; serialize all calls.
 * 单任务 CC1101 SPI 驱动；全部调用必须串行。
 */
class SpiDevice {
 public:
  /**
   * @brief Reset the radio and initialize verified pins without transmitting.
   * 复位模块并初始化已验证引脚，不发射。
   * @return None on success; readiness/reset error otherwise. / 成功返回 None，否则返回就绪或复位错误。
   */
  BusError begin();
  /**
   * @brief Read identity and IDLE status with separate status commands.
   * 通过独立状态命令读取身份与 IDLE 状态。
   * @param result Caller-owned output; may be partial on failure. / 调用方输出；失败时可能不完整。
   * @return SPI result only; call validate() to check identity. / 仅为 SPI 结果；身份需另行 validate()。
   */
  BusError readSnapshot(Snapshot& result);
  /**
   * @brief Configure asynchronous OOK and enter RX; GDO0 becomes a radio output.
   * 配置异步 OOK 并进入 RX；GDO0 变为模块输出。
   * @param frequencyHz Nominal Hz within 430000000..437000000. / 名义频率 Hz，范围 430000000..437000000。
   * @param profileIndex Index below radio_config::kProfileCount. / 小于 kProfileCount 的配置索引。
   * @return None after configuration readback and confirmed RX, else first error. / 配置回读及 RX 确认成功返回 None，否则首个错误。
   */
  BusError configureReceive(uint32_t frequencyHz, std::size_t profileIndex);
  /**
   * @brief Enter RX using the existing receive configuration.
   * 使用现有接收配置进入 RX。
   * @return None after two confirmed RX reads, else bus/state error. / 两次确认 RX 返回 None，否则总线或状态错误。
   */
  BusError startReceive();
  /**
   * @brief Prepare fixed OOK/PATABLE settings with GDO0 high impedance; no STX.
   * 准备固定 OOK 与 PATABLE，GDO0 三态；不执行 STX。
   * @return None after verified writes/readback, else first error. / 写入回读成功返回 None，否则首个错误。
   */
  BusError configureTransmit();
  /**
   * @brief Enter TX; configureTransmit() must succeed and ESP32 must already drive GDO0 low.
   * 进入 TX；configureTransmit() 须已成功，ESP32 须已将 GDO0 驱动为低。
   * @return None after confirmed TX, else configuration/bus/state error. / 确认 TX 返回 None，否则配置、总线或状态错误。
   */
  BusError startTransmit();
  /**
   * @brief Request and confirm IDLE; stop envelope generation before calling.
   * 请求并确认 IDLE；调用前先停止包络生成。
   * @return None after two IDLE reads, else bus/state error. / 两次 IDLE 确认返回 None，否则总线或状态错误。
   */
  BusError stopTransmit();
  /**
   * @brief Request SIDLE and confirm two consecutive IDLE reads.
   * 请求 SIDLE 并连续两次确认 IDLE。
   * @return None on confirmed IDLE, else bus/state error. / 确认 IDLE 返回 None，否则总线或状态错误。
   */
  BusError stopReceive();
  /**
   * @brief Read raw radio status; use all output fields only after success.
   * 读取原始模块状态；仅成功后才能使用全部输出字段。
   * @param result Caller-owned output; may be partial on failure. / 调用方输出；失败时可能不完整。
   * @return None on success, else bus error; output may be partial. / 成功返回 None，否则总线错误；输出可能不完整。
   */
  BusError readRuntimeStatus(RuntimeStatus& result);
  /**
   * @brief Read uncalibrated raw RSSI.
   * 读取未经校准的原始 RSSI。
   * @param raw Raw RSSI output, valid only on success. / 原始 RSSI 输出，仅成功时有效。
   * @return None on success; output is undefined after failure. / 成功返回 None；失败后输出不可用。
   */
  BusError readRssi(uint8_t& raw);
  /**
   * @brief Return the last attempted SPI command for diagnostics.
   * 返回最近尝试的 SPI 命令供诊断。
   */
  uint8_t lastCommand() const { return lastCommand_; }

 private:
  BusError configureOok(uint32_t frequencyHz, std::size_t profileIndex, bool receive);
  BusError writePaTable();
  bool waitReady() const;
  BusError select(uint8_t command);
  BusError readStatus(uint8_t address, uint8_t& value, uint8_t& chipStatus);
  BusError readConfig(uint8_t address, uint8_t& value);
  BusError writeVerified(uint8_t address, uint8_t value);
  BusError strobe(uint8_t command);
  BusError waitState(uint8_t expected);
  void endAccess();

  SPIClass spi_{VSPI};
  bool initialized_ = false;
  bool transmitConfigured_ = false;
  uint8_t lastCommand_ = 0;
};
}  // namespace cc1101
