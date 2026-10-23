// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <cstring>
#include "cc1101_spi.h"
#include "config.h"
#include "remote_frames.h"
#include "rf_replay.h"

namespace {
cc1101::SpiDevice radio;
replay::Transmitter transmitter;
replay::Cooldown cooldown;
bool ready = false;
uint32_t attempts = 0;
char command[64]{};
std::size_t used = 0;
bool overlong = false;

void status() {
  cc1101::RuntimeStatus value;
  const auto bus = radio.readRuntimeStatus(value);
  Serial.printf(
      "{\"type\":\"status\",\"phase\":3,\"ready\":%s,\"bus_error\":%d,"
      "\"marcstate\":%u,\"frequency_hz\":%lu,\"nominal_power_dbm\":%d,"
      "\"max_repeats\":%u,\"plan_us\":%lu,\"attempts\":%lu,\"cooldown_ready\":%s,"
      "\"buttons\":[\"power\",\"brightness_up\",\"brightness_down\",\"cct_up\",\"cct_down\","
      "\"night_mode\",\"timer_10min\"]}\n",
      ready ? "true" : "false", static_cast<int>(bus), value.marcstate,
      static_cast<unsigned long>(radio_config::kDefaultFrequencyHz),
      replay_config::kNominalPowerDbm, replay_config::kRepeats,
      static_cast<unsigned long>(transmitter.plan().durationUs),
      static_cast<unsigned long>(attempts), cooldown.available(millis()) ? "true" : "false");
}

void execute(const char* line) {
  if (std::strcmp(line, "status") == 0) {
    status();
    return;
  }
  const auto* frame = std::strncmp(line, "send ", 5) == 0 ? replay::findFrame(line + 5) : nullptr;
  if (!frame) {
    Serial.println("{\"type\":\"error\",\"message\":\"unknown_command\"}");
    return;
  }
  if (!ready || !cooldown.available(millis())) {
    Serial.println("{\"type\":\"error\",\"message\":\"not_ready_or_cooldown\"}");
    return;
  }
  cooldown.attempted(millis());
  ++attempts;
  Serial.printf("{\"type\":\"tx_begin\",\"button\":\"%s\",\"hex\":\"%s\",\"attempt\":%lu}\n",
                frame->name, frame->hex, static_cast<unsigned long>(attempts));
  Serial.flush();
  const auto result = transmitter.send(*frame, radio);
  cooldown.attempted(
      millis());  // Apply at least 20 ms cooldown after this attempt ends / 从本次结束再冷却至少 20 毫秒
  if (!result.completed())
    ready =
        false;  // Latch failures until manual diagnosis/reset; never retry automatically / 失败锁定，必须人工诊断后复位，不自动重试
  // Discard serial commands queued during TX to avoid repeated toggles from pasted input.
  // 不执行发送期间积压的串口命令，避免粘贴/重复点击造成连续开关。
  while (Serial.available()) Serial.read();
  Serial.printf(
      "{\"type\":\"tx_result\",\"button\":\"%s\",\"attempt\":%lu,"
      "\"waveform_completed\":%s,\"radio_error\":%d,\"idle_error\":%d,\"rmt_error\":%d,"
      "\"tx_marcstate\":%u,\"final_marcstate\":%u,\"elapsed_us\":%lu,"
      "\"lamp_effect\":\"awaiting_operator_observation\"}\n",
      frame->name, static_cast<unsigned long>(attempts), result.completed() ? "true" : "false",
      static_cast<int>(result.radioError), static_cast<int>(result.idleError),
      static_cast<int>(result.timingError), result.txState, result.finalState,
      static_cast<unsigned long>(result.elapsedUs));
}
}  // namespace

void setup() {
  Serial.begin(config::kSerialBaud);
  delay(300);
  auto error = radio.begin();
  cc1101::Snapshot identity;
  if (error == cc1101::BusError::None) error = radio.readSnapshot(identity);
  const auto identityError = cc1101::validate(identity);
  Serial.printf(
      "{\"type\":\"boot\",\"phase\":3,\"bus_error\":%d,\"identity_error\":%d,"
      "\"partnum\":%u,\"version\":%u,\"automatic_transmit\":false}\n",
      static_cast<int>(error), static_cast<int>(identityError), identity.partnum, identity.version);
  if (error != cc1101::BusError::None || identityError != cc1101::IdentityError::None) return;
  ready = transmitter.begin(radio);
  Serial.printf("{\"type\":\"tx_setup\",\"ready\":%s,\"last_spi_command\":%u}\n",
                ready ? "true" : "false", radio.lastCommand());
  status();
}

void loop() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') {
      if (overlong)
        Serial.println("{\"type\":\"error\",\"message\":\"command_too_long\"}");
      else if (used) {
        command[used] = '\0';
        execute(command);
      }
      used = 0;
      overlong = false;
    } else if (used < sizeof(command) - 1)
      command[used++] = c;
    else
      overlong = true;
  }
  delay(2);
}
