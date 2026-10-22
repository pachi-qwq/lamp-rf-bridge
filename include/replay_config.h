// SPDX-License-Identifier: MIT
#pragma once
#include <cstddef>
#include <cstdint>

namespace replay_config {
// Use only captured lamp commands; serial exposes no arbitrary frequency, power or payload interface.
// 仅用于已采集的本机灯具命令；不提供任意频率、功率或数据的串口接口。
inline constexpr uint8_t kRepeats =
    3;  // Observed repetition count; six command types were lamp-tested / 已观察到的帧重复范围，六类按键已实测
inline constexpr uint8_t kMaxRepeats = 3;
inline constexpr uint8_t kAdjustmentRepeats =
    2;  // Short adjustment burst; not every key was physically retested at two frames / 常用调节短发，需实体确认
inline constexpr uint8_t kPaOn =
    0x34;  // TI table 39: typical -10 dBm at 433 MHz, not measured radiated power / TI 表 39：433 MHz 典型 -10 dBm，非实测辐射功率
inline constexpr int kNominalPowerDbm = -10;
inline constexpr uint32_t kLeadLowUs = 14000;
inline constexpr uint32_t kTailLowUs =
    3000;  // Allow low-level propagation through the asynchronous modulation path / 给异步调制链路留出低电平传播时间
inline constexpr uint32_t kMaxPlanUs = 200000;
inline constexpr uint32_t kWaitTimeoutMs = 300;
inline constexpr uint32_t kCooldownMs = 20;
inline constexpr uint8_t kRmtChannel = 0;
inline constexpr uint8_t kRmtBlocks =
    2;  // 128 items hold the entire transmission without mid-stream refill / 128 项，整次发送一次装入硬件，不依赖中途补数
inline constexpr uint8_t kClockDivider = 80;  // APB 80 MHz / 80 = 1 us / APB 80 MHz / 80 = 1 us
inline constexpr std::size_t kFramePulses = 50;
inline constexpr std::size_t kMaxPulses = 2 + kFramePulses * kMaxRepeats;
}  // namespace replay_config
