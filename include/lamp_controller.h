// SPDX-License-Identifier: MIT
#pragma once
#include "cc1101_spi.h"
#include "rf_replay.h"

namespace lamp {
/**
 * @brief Dispatch outcome; Sent means waveform completion, not lamp confirmation.
 * 分发结果；Sent 表示波形完成，不是灯具确认。
 */
enum class SendCode { Sent, UnknownButton, NotReady, Cooldown, HardwareError };
/**
 * @brief Own SPI/RMT and the fault latch; all calls run in the Arduino task.
 * 管理 SPI、RMT 与故障锁定；全部调用在 Arduino 任务执行。
 */
class Controller {
 public:
  /**
   * @brief Initialize identity, RMT and IDLE health without transmitting.
   * 初始化芯片身份、RMT 与 IDLE 健康检查，不发射。
   * @return True only when ready; errors require diagnosis/reset. / 仅就绪时 true；错误需诊断及复位。
   */
  bool begin();
  /**
   * @brief Check real radio IDLE state and latch faults; never auto-recover.
   * 检查真实模块 IDLE 状态并锁定故障；不自动恢复。
   * @return Current readiness, not lamp state. / 当前就绪状态，不是灯具状态。
   */
  bool healthy();
  /**
   * @brief Dispatch one exact built-in name with cooldown and no retry.
   * 按精确内置名称分发一次，执行冷却且不重试。
   * @param button Non-null NUL-terminated built-in command name. / 非空且 NUL 结尾的内置命令名。
   * @return Sent, UnknownButton, NotReady, Cooldown or HardwareError. / 已发送、未知按键、未就绪、冷却或硬件错误。
   */
  SendCode send(const char* button);
  /**
   * @brief Return cached initialization/fault readiness, not lamp state.
   * 返回缓存的初始化及故障就绪状态，不是灯具状态。
   */
  bool ready() const { return ready_; }
  /**
   * @brief Return this boot's actual TX attempts, including failures.
   * 返回本次启动的实际发射尝试次数，包括失败。
   */
  uint32_t attempts() const { return attempts_; }
  /**
   * @brief Return remaining cooldown milliseconds after the last attempt ended.
   * 返回最近尝试结束后的剩余冷却毫秒数。
   */
  uint32_t cooldownMs() const;
  /**
   * @brief Return the last TX result; meaningful only after an attempt.
   * 返回最近发射结果；发生过尝试后才有意义。
   */
  const replay::Result& lastResult() const { return lastResult_; }
  /**
   * @brief Return the last attempted name, or empty before the first attempt.
   * 返回最近尝试名称，首次尝试前为空。
   */
  const char* lastButton() const { return lastButton_; }
  /**
   * @brief Return cached radio MARCSTATE; 0xFF means no valid reading.
   * 返回缓存的模块 MARCSTATE；0xFF 表示无有效读数。
   */
  uint8_t marcstate() const { return marcstate_; }
  /**
   * @brief Return the last health-check BusError numeric value.
   * 返回最近健康检查的 BusError 数值。
   */
  int busError() const { return busError_; }

 private:
  cc1101::SpiDevice radio_;
  replay::Transmitter transmitter_;
  replay::Cooldown cooldown_;
  replay::Result lastResult_;
  bool ready_ = false;
  uint32_t attempts_ = 0;
  uint32_t lastEndMs_ = 0;
  uint8_t marcstate_ = 0xFF;
  int busError_ = 0;
  const char* lastButton_ = "";
};
}  // namespace lamp
