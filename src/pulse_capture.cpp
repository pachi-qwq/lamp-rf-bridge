// SPDX-License-Identifier: MIT
#include "pulse_capture.h"
#include <driver/gpio.h>
#include <esp_intr_alloc.h>
#include <esp_timer.h>
#include <soc/gpio_struct.h>
#include "config.h"

namespace capture {
static_assert(config::pins::kGdo0 < 32, "ISR 当前读取 GPIO.in，只支持 GPIO0..31");
static_assert((kPreCapacity & (kPreCapacity - 1)) == 0, "预触发容量须为 2 的幂");

esp_err_t PulseCapture::begin() {
  gpio_config_t options{};
  options.pin_bit_mask = 1ULL << config::pins::kGdo0;
  options.mode = GPIO_MODE_INPUT;
  options.intr_type = GPIO_INTR_DISABLE;
  auto error = gpio_config(&options);
  if (error != ESP_OK) return error;
  error = gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  if (error != ESP_OK) return error;
  return gpio_isr_handler_add(static_cast<gpio_num_t>(config::pins::kGdo0), onEdge, this);
}

void PulseCapture::arm(bool waitForTrigger) {
  gpio_intr_disable(static_cast<gpio_num_t>(config::pins::kGdo0));
  portENTER_CRITICAL(&lock_);
  preWrite_ = preCount_ = postCount_ = ambiguousEdges_ = zeroDurations_ = 0;
  clipped_ = false;
  startedUs_ = lastEdgeUs_ = static_cast<uint32_t>(esp_timer_get_time());
  lastLevel_ = (GPIO.in >> config::pins::kGdo0) & 1U;
  state_ = waitForTrigger ? State::Waiting : State::Recording;
  portEXIT_CRITICAL(&lock_);
  gpio_set_intr_type(static_cast<gpio_num_t>(config::pins::kGdo0), GPIO_INTR_ANYEDGE);
  gpio_intr_enable(static_cast<gpio_num_t>(config::pins::kGdo0));
}

void PulseCapture::trigger() {
  portENTER_CRITICAL(&lock_);
  if (state_ == State::Waiting) state_ = State::Recording;
  portEXIT_CRITICAL(&lock_);
}

State PulseCapture::state() {
  portENTER_CRITICAL(&lock_);
  const State result = state_;
  portEXIT_CRITICAL(&lock_);
  return result;
}

void IRAM_ATTR PulseCapture::onEdge(void* argument) {
  auto* self = static_cast<PulseCapture*>(argument);
  const uint32_t now = static_cast<uint32_t>(esp_timer_get_time());
  const uint8_t nextLevel = (GPIO.in >> config::pins::kGdo0) & 1U;
  portENTER_CRITICAL_ISR(&self->lock_);
  if (self->state_ != State::Waiting && self->state_ != State::Recording) {
    portEXIT_CRITICAL_ISR(&self->lock_);
    return;
  }
  // Repeated same-level callbacks may indicate glitches or merged edges; flag possible damage.
  // 相同电平的重复回调可能意味着短毛刺或合并边沿，明确标记数据可能受损。
  if (nextLevel == self->lastLevel_) {
    ++self->ambiguousEdges_;
    portEXIT_CRITICAL_ISR(&self->lock_);
    return;
  }
  const uint32_t elapsed = now - self->lastEdgeUs_;
  if (elapsed == 0) ++self->zeroDurations_;
  const uint32_t packed = (elapsed << 1) | self->lastLevel_;
  if (self->state_ == State::Waiting) {
    self->pre_[self->preWrite_] = packed;
    self->preWrite_ = (self->preWrite_ + 1) & (kPreCapacity - 1);
    if (self->preCount_ < kPreCapacity)
      ++self->preCount_;
    else
      self->clipped_ = true;
  } else {
    self->post_[self->postCount_++] = packed;
    if (self->postCount_ == kPostCapacity) {
      self->state_ = State::Full;
      self->endedUs_ = now;
    }
  }
  self->lastEdgeUs_ = now;
  self->lastLevel_ = nextLevel;
  portEXIT_CRITICAL_ISR(&self->lock_);
}

Summary PulseCapture::stop() {
  gpio_intr_disable(static_cast<gpio_num_t>(config::pins::kGdo0));
  portENTER_CRITICAL(&lock_);
  const bool full = state_ == State::Full;
  const uint32_t ended = full ? endedUs_ : static_cast<uint32_t>(esp_timer_get_time());
  Summary result{preCount_,
                 postCount_,
                 ambiguousEdges_,
                 zeroDurations_,
                 ended - startedUs_,
                 ended - lastEdgeUs_,
                 lastLevel_,
                 full,
                 clipped_};
  state_ = State::Stopped;
  portEXIT_CRITICAL(&lock_);
  return result;
}

uint32_t PulseCapture::at(std::size_t index) const {
  if (index < preCount_) {
    return pre_[(preWrite_ + kPreCapacity - preCount_ + index) & (kPreCapacity - 1)];
  }
  return post_[index - preCount_];
}
}  // namespace capture
