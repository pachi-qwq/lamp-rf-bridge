// SPDX-License-Identifier: MIT
#include "rf_replay.h"
#include <Arduino.h>
#include "config.h"
#include "remote_frames.h"

namespace replay {
namespace {
constexpr auto kChannel = static_cast<rmt_channel_t>(replay_config::kRmtChannel);
static_assert(replay_config::kMaxPulses / 2 + 1 < replay_config::kRmtBlocks * 64,
              "整次波形及终止标记必须装入 RMT 内存");
}  // namespace

bool Transmitter::begin(cc1101::SpiDevice& radio) {
  // Make CC1101 GDO0 high impedance before enabling ESP32 output to avoid bus contention.
  // 必须先让 CC1101 GDO0 三态，再让 ESP32 输出，防止两个输出端互相驱动。
  if (radio.configureTransmit() != cc1101::BusError::None) return false;
  for (const auto& frame : kFrames) {
    if (!buildPlan(frame.pulses, replay_config::kFramePulses, frame.repeats, plan_)) return false;
  }
  rmt_config_t settings{};
  settings.rmt_mode = RMT_MODE_TX;
  settings.channel = kChannel;
  settings.gpio_num = static_cast<gpio_num_t>(config::pins::kGdo0);
  settings.clk_div = replay_config::kClockDivider;
  settings.mem_block_num = replay_config::kRmtBlocks;
  settings.tx_config.loop_en = false;
  settings.tx_config.carrier_en =
      false;  // RMT generates only the envelope; CC1101 generates the 433 MHz carrier / RMT 只产生包络；433 MHz 载波由 CC1101 生成
  settings.tx_config.idle_output_en = true;
  settings.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  if (rmt_config(&settings) != ESP_OK) return false;
  if (rmt_driver_install(kChannel, 0, 0) != ESP_OK) return false;
  uint32_t clock = 0;
  if (rmt_get_counter_clock(kChannel, &clock) != ESP_OK || clock != 1000000) {
    rmt_driver_uninstall(kChannel);
    return false;
  }
  initialized_ = true;
  return true;
}

Result Transmitter::send(const Frame& frame, cc1101::SpiDevice& radio) {
  Result result;
  if (!initialized_) {
    result.timingError = ESP_ERR_INVALID_STATE;
    return result;
  }
  if (!buildPlan(frame.pulses, replay_config::kFramePulses, frame.repeats, plan_)) {
    result.timingError = ESP_ERR_INVALID_ARG;
    return result;
  }
  for (std::size_t i = 0; i < plan_.count / 2; ++i) {
    items_[i].duration0 = plan_.pulses[2 * i].durationUs;
    items_[i].level0 = plan_.pulses[2 * i].level;
    items_[i].duration1 = plan_.pulses[2 * i + 1].durationUs;
    items_[i].level1 = plan_.pulses[2 * i + 1].level;
  }
  const uint32_t started = micros();
  result.radioError = radio.startTransmit();
  if (result.radioError == cc1101::BusError::None) {
    cc1101::RuntimeStatus tx;
    result.radioError = radio.readRuntimeStatus(tx);
    result.txState = tx.marcstate;
    if (result.radioError == cc1101::BusError::None && result.txState != 0x13)
      result.radioError = cc1101::BusError::StateTimeout;
  }
  if (result.radioError == cc1101::BusError::None) {
    // Load a finite non-looping waveform; it ends low even if the CPU stalls (PATABLE[0]=0).
    // 有限波形一次装入，不开启循环；CPU 停顿后也会停在低电平（PATABLE[0]=0）。
    result.timingError =
        rmt_write_items(kChannel, items_, static_cast<int>(plan_.count / 2), false);
    if (result.timingError == ESP_OK)
      result.timingError = rmt_wait_tx_done(kChannel, pdMS_TO_TICKS(replay_config::kWaitTimeoutMs));
  }
  // Stop on success, timeout or SPI error; drive data low before leaving TX.
  // 正常、超时、SPI 错误均执行停止。先让数据线低，再结束 TX。
  const auto stop = rmt_tx_stop(kChannel);
  if (result.timingError == ESP_OK) result.timingError = stop;
  result.idleError = radio.stopTransmit();
  cc1101::RuntimeStatus idle;
  const auto status = radio.readRuntimeStatus(idle);
  if (result.idleError == cc1101::BusError::None) result.idleError = status;
  if (status == cc1101::BusError::None) result.finalState = idle.marcstate;
  result.elapsedUs = static_cast<uint32_t>(micros() - started);
  return result;
}
}  // namespace replay
