// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>
#include "replay_config.h"

namespace replay {
/**
 * @brief One envelope level and duration in microseconds.
 * 单段包络电平及微秒持续时间。
 */
struct Pulse {
  uint16_t durationUs;
  uint8_t level;
};
/**
 * @brief Finite replay storage, pulse count and total duration in microseconds.
 * 有限重放缓冲、脉冲数与微秒总时长。
 */
struct Plan {
  Pulse pulses[replay_config::kMaxPulses]{};
  std::size_t count = 0;
  uint32_t durationUs = 0;
};

// Validate frame boundaries, alternating levels and timing; failure leaves no sendable plan.
// 纯逻辑：检查原始帧边界、交替电平和时长；失败不生成可发送的计划。
/**
 * @brief Build a bounded plan from 50 alternating pulses; payload 200..1600 us, final low 10..20 ms.
 * 从 50 段交替脉冲生成有界计划；数据段 200..1600us，末低电平 10..20ms。
 * @param frame Captured frame; application enforces the command whitelist. / 已采集帧；应用负责命令白名单。
 * @param count Must equal replay_config::kFramePulses (50). / 须等于 kFramePulses（50）。
 * @param repeats 1..replay_config::kMaxRepeats. / 1..kMaxRepeats。
 * @param out Cleared first; use only on true. / 先清零；仅 true 时可用。
 * @return True for a valid bounded plan; output is cleared first and usable only on true. / 合法有界计划返回 true；先清零输出，仅 true 时可用。
 */
inline bool buildPlan(const Pulse* frame, std::size_t count, uint8_t repeats, Plan& out) {
  out = {};
  if (!frame || count != replay_config::kFramePulses || repeats == 0 ||
      repeats > replay_config::kMaxRepeats)
    return false;
  uint32_t frameUs = 0;
  for (std::size_t i = 0; i < count; ++i) {
    const auto& pulse = frame[i];
    if (pulse.level != (i % 2 == 0 ? 1 : 0)) return false;
    const bool delimiter = i == count - 1;
    if (delimiter ? (pulse.durationUs < 10000 || pulse.durationUs > 20000)
                  : (pulse.durationUs < 200 || pulse.durationUs > 1600))
      return false;
    frameUs += pulse.durationUs;
  }
  const uint32_t total = replay_config::kLeadLowUs + frameUs * repeats + replay_config::kTailLowUs;
  if (total > replay_config::kMaxPlanUs) return false;
  out.pulses[out.count++] = {static_cast<uint16_t>(replay_config::kLeadLowUs), 0};
  for (uint8_t n = 0; n < repeats; ++n) {
    for (std::size_t i = 0; i < count; ++i) out.pulses[out.count++] = frame[i];
  }
  out.pulses[out.count++] = {static_cast<uint16_t>(replay_config::kTailLowUs), 0};
  out.durationUs = total;
  return true;
}

// Every TX attempt, including failure, starts cooldown; unsigned elapsed time tolerates millis() wrap.
// 所有发射尝试（包含失败）都触发冷却；无符号差值允许 millis() 回绕。
/**
 * @brief Elapsed-time gate applied to every TX attempt, including failures.
 * 每次发射尝试均执行的时间间隔限制，包括失败。
 */
class Cooldown {
 public:
  /**
   * @brief Check whether kCooldownMs has elapsed using unsigned millis() subtraction.
   * 通过无符号 millis() 差检查是否已经过 kCooldownMs。
   * @param now Current unsigned millis() value. / 当前无符号 millis() 值。
   * @return True if no prior attempt exists or cooldown has elapsed. / 尚未尝试或已过冷却时 true。
   */
  bool available(uint32_t now) const {
    return !attempted_ || static_cast<uint32_t>(now - last_) >= replay_config::kCooldownMs;
  }
  /**
   * @brief Record the current attempt/start-or-end time, even on failure.
   * 记录当前尝试开始或结束时间，包括失败。
   * @param now Current unsigned millis() value. / 当前无符号 millis() 值。
   */
  void attempted(uint32_t now) {
    attempted_ = true;
    last_ = now;
  }

 private:
  bool attempted_ = false;
  uint32_t last_ = 0;
};
}  // namespace replay
