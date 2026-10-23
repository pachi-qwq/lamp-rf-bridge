// SPDX-License-Identifier: MIT
#pragma once
#include <driver/rmt.h>
#include "cc1101_spi.h"
#include "replay_plan.h"
#include "remote_frames.h"

namespace replay {
/**
 * @brief Radio/RMT outcome and elapsed microseconds; not a lamp acknowledgement.
 * 射频与 RMT 结果及微秒耗时；不是灯具确认。
 */
struct Result {
  cc1101::BusError radioError = cc1101::BusError::None;
  cc1101::BusError idleError = cc1101::BusError::None;
  esp_err_t timingError = ESP_OK;
  uint8_t txState = 0;
  uint8_t finalState = 0xFF;
  uint32_t elapsedUs = 0;
  /**
   * @brief Check timing success, observed TX and confirmed final IDLE.
   * 检查时序成功、观察到 TX 及最终确认 IDLE。
   */
  bool completed() const {
    return radioError == cc1101::BusError::None && idleError == cc1101::BusError::None &&
           timingError == ESP_OK && txState == 0x13 && finalState == cc1101::registers::kIdle;
  }
};

// The applications expose seven captured commands; startup, reset and status never transmit.
// 应用只开放已标定的七个命令；启动、复位和 status 均不发射。
/**
 * @brief Finite hardware-RMT OOK replay; non-reentrant, single-task, begin once.
 * 硬件 RMT 有限 OOK 重放；不可重入、单任务，begin 一次。
 */
class Transmitter {
 public:
  /**
   * @brief Prepare radio and 1 MHz RMT without transmitting; tri-state radio before ESP32 output.
   * 准备射频与 1MHz RMT，不发射；ESP32 输出前模块先三态。
   * @param radio Initialized driver used by begin; must outlive this call. / begin 所用的已初始化驱动；生命周期须覆盖调用。
   * @return True only after configuration and built-in plan validation. / 仅配置与内置计划校验成功时 true。
   */
  bool begin(cc1101::SpiDevice& radio);
  /**
   * @brief For a valid initialized plan, transmit then stop RMT and request/check IDLE.
   * 初始化及计划合法时发射，尝试结束后停止 RMT 并请求、检查 IDLE。
   * @param frame Captured frame; application enforces the command whitelist. / 已采集帧；应用负责命令白名单。
   * @param radio Initialized driver used by begin; must outlive this call. / begin 所用的已初始化驱动；生命周期须覆盖调用。
   * @return Detailed result; completed() does not confirm the lamp response. / 详细结果；completed() 不确认灯具效果。
   */
  Result send(const Frame& frame, cc1101::SpiDevice& radio);
  /**
   * @brief Return the latest plan by reference, valid until the next build.
   * 按引用返回最近计划，下次生成前有效。
   */
  const Plan& plan() const { return plan_; }

 private:
  Plan plan_{};
  rmt_item32_t items_[replay_config::kMaxPulses / 2]{};
  bool initialized_ = false;
};
}  // namespace replay
