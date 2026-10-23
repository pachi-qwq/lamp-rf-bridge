// SPDX-License-Identifier: MIT
#pragma once
#include <Arduino.h>
#include <cstddef>
#include <cstdint>
#include <esp_err.h>

namespace capture {
// Use fixed storage; the ISR allocates no memory and performs no serial or SPI access.
// 固定内存，ISR 不申请内存、不打印串口、不调用 SPI。
// While awaiting RSSI, retain only the latest 1024 pulses so idle noise cannot exhaust storage.
// 等待 RSSI 触发时只保留最近 1024 个脉冲，防止空闲噪声耗尽整段缓冲。
inline constexpr std::size_t kPreCapacity = 1024;
inline constexpr std::size_t kPostCapacity = 16384;
/**
 * @brief Capture lifecycle; Full indicates exhausted post-trigger storage.
 * 捕获生命周期；Full 表示触发后缓冲已满。
 */
enum class State { Stopped, Waiting, Recording, Full };

/**
 * @brief Stopped-capture counts, quality flags and microsecond durations; unfinished tail is separate.
 * 停止后的计数、质量标志与微秒时长；未完成尾段单独记录。
 */
struct Summary {
  uint32_t preCount;
  uint32_t postCount;
  uint32_t ambiguousEdges;
  uint32_t zeroDurations;
  uint32_t elapsedUs;
  uint32_t tailUs;
  uint8_t tailLevel;
  bool overflow;
  bool prehistoryClipped;
};

// Packing saves RAM only; export explicit levels and durations in microseconds.
// 打包仅用于节省 RAM；导出时还原为明确的电平与微秒持续时间。
/**
 * @brief Decode pulse duration in microseconds from the packed value.
 * 从打包值解码脉冲微秒时长。
 * @param packed Packed pulse from this capture format. / 本采集格式的打包脉冲。
 */
constexpr uint32_t durationUs(uint32_t packed) {
  return packed >> 1;
}
/**
 * @brief Decode level 0/1 from the packed pulse's lowest bit.
 * 从打包脉冲最低位解码电平 0/1。
 * @param packed Packed pulse from this capture format. / 本采集格式的打包脉冲。
 */
constexpr uint8_t level(uint32_t packed) {
  return packed & 1U;
}

/**
 * @brief Fixed-buffer GPIO edge timestamps; single capture consumer.
 * 采用固定缓冲的 GPIO 边沿时间戳采集；单一消费者。
 */
class PulseCapture {
 public:
  /**
   * @brief Install an IRAM ISR on configured GDO0; call once before arm().
   * 在配置的 GDO0 安装 IRAM ISR；arm() 前调用一次。
   * @return ESP_OK or the GPIO/ISR installation error. / ESP_OK 或 GPIO、ISR 安装错误。
   */
  esp_err_t begin();
  /**
   * @brief Reset buffers and enable both-edge interrupts.
   * 清空缓冲并开启双边沿中断。
   * @param waitForTrigger True waits for trigger(); false records immediately. / true 等待 trigger()，false 立即记录。
   */
  void arm(bool waitForTrigger);
  /**
   * @brief Move Waiting to Recording; leave other states unchanged.
   * 将 Waiting 改为 Recording；其他状态不变。
   */
  void trigger();
  /**
   * @brief Read capture state under the ISR lock.
   * 在 ISR 锁保护下读取捕获状态。
   */
  State state();
  /**
   * @brief Disable capture and return counts/quality; call before count()/at().
   * 停止采集并返回计数与质量；count()/at() 前调用。
   */
  Summary stop();
  /**
   * @brief Read stored pulse count after stop(); excludes the unfinished tail.
   * 读取 stop() 后已存脉冲数，不含未完成尾段。
   */
  std::size_t count() const { return preCount_ + postCount_; }
  /**
   * @brief Read a packed pulse chronologically after stop(); no bounds checking.
   * stop() 后按时间顺序读取打包脉冲；不检查越界。
   * @param index Must be below count(); only read after stop(). / 须小于 count()；仅停止后读取。
   */
  uint32_t at(std::size_t index) const;

 private:
  static void IRAM_ATTR onEdge(void* argument);
  portMUX_TYPE lock_ = portMUX_INITIALIZER_UNLOCKED;
  State state_ = State::Stopped;
  uint32_t pre_[kPreCapacity]{};
  uint32_t post_[kPostCapacity]{};
  uint32_t preWrite_ = 0;
  uint32_t preCount_ = 0;
  uint32_t postCount_ = 0;
  uint32_t ambiguousEdges_ = 0;
  uint32_t zeroDurations_ = 0;
  uint32_t lastEdgeUs_ = 0;
  uint32_t startedUs_ = 0;
  uint32_t endedUs_ = 0;
  uint8_t lastLevel_ = 0;
  bool clipped_ = false;
};
}  // namespace capture
