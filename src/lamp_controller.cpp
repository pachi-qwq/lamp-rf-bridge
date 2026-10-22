// SPDX-License-Identifier: MIT
#include "lamp_controller.h"
#include <Arduino.h>

namespace lamp {
bool Controller::begin() {
  auto error = radio_.begin();
  cc1101::Snapshot identity;
  if (error == cc1101::BusError::None) error = radio_.readSnapshot(identity);
  const auto validation = cc1101::validate(identity);
  Serial.printf(
      "{\"type\":\"rf_boot\",\"bus_error\":%d,\"identity_error\":%d,"
      "\"partnum\":%u,\"version\":%u,\"automatic_transmit\":false}\n",
      static_cast<int>(error), static_cast<int>(validation), identity.partnum, identity.version);
  if (error != cc1101::BusError::None || validation != cc1101::IdentityError::None) return false;
  ready_ = transmitter_.begin(radio_);
  return healthy();
}

bool Controller::healthy() {
  cc1101::RuntimeStatus state;
  const auto error = radio_.readRuntimeStatus(state);
  busError_ = static_cast<int>(error);
  marcstate_ = error == cc1101::BusError::None ? state.marcstate : 0xFF;
  if (error != cc1101::BusError::None || marcstate_ != cc1101::registers::kIdle) {
    if (ready_)
      Serial.printf("{\"type\":\"rf_fault\",\"bus_error\":%d,\"marcstate\":%u}\n", busError_,
                    marcstate_);
    ready_ = false;
    radio_.stopTransmit();
  }
  return ready_;
}

uint32_t Controller::cooldownMs() const {
  if (!attempts_) return 0;
  const uint32_t elapsed = static_cast<uint32_t>(millis() - lastEndMs_);
  return elapsed < replay_config::kCooldownMs ? replay_config::kCooldownMs - elapsed : 0;
}

SendCode Controller::send(const char* name) {
  const auto* frame = replay::findFrame(name);
  if (!frame) return SendCode::UnknownButton;
  if (!ready_ || !healthy()) return SendCode::NotReady;
  if (!cooldown_.available(millis())) return SendCode::Cooldown;
  cooldown_.attempted(millis());
  ++attempts_;
  lastButton_ = frame->name;
  lastResult_ = transmitter_.send(*frame, radio_);
  lastEndMs_ = millis();
  cooldown_.attempted(lastEndMs_);
  ready_ = lastResult_.completed();
  marcstate_ = lastResult_.finalState;
  Serial.printf(
      "{\"type\":\"tx_result\",\"phase\":4,\"button\":\"%s\",\"attempt\":%lu,"
      "\"waveform_completed\":%s,\"radio_error\":%d,\"idle_error\":%d,"
      "\"rmt_error\":%d,\"final_marcstate\":%u}\n",
      lastButton_, static_cast<unsigned long>(attempts_), ready_ ? "true" : "false",
      static_cast<int>(lastResult_.radioError), static_cast<int>(lastResult_.idleError),
      static_cast<int>(lastResult_.timingError), marcstate_);
  return ready_ ? SendCode::Sent : SendCode::HardwareError;
}
}  // namespace lamp
