// SPDX-License-Identifier: MIT
#include <Arduino.h>
#include <cstring>
#include <cstdlib>
#include "cc1101_spi.h"
#include "config.h"
#include "pulse_capture.h"
#include "radio_config.h"

namespace {
cc1101::SpiDevice radio;
capture::PulseCapture pulses;
bool ready = false;
bool capturing = false;
bool triggered = false;
uint32_t frequencyHz = radio_config::kDefaultFrequencyHz;
std::size_t profileIndex = 0;
uint32_t armedMs = 0;
uint32_t triggeredMs = 0;
uint32_t durationMs = 0;
uint32_t waitMs = 0;
uint32_t lastRssiMs = 0;
int triggerDbmX2 = -140;
int peakDbmX2 = -1000;
int triggerActualX2 = -1000;
uint32_t carrierSamples = 0;
char command[100]{};
std::size_t commandSize = 0;
bool commandTooLong = false;

void error(cc1101::BusError code) {
  Serial.printf("{\"type\":\"error\",\"bus_error\":%d,\"command\":%u}\n", static_cast<int>(code),
                radio.lastCommand());
}

void status() {
  cc1101::RuntimeStatus value;
  const auto result = radio.readRuntimeStatus(value);
  if (result != cc1101::BusError::None) {
    error(result);
    return;
  }
  const auto& profile = radio_config::kProfiles[profileIndex];
  Serial.printf(
      "{\"type\":\"status\",\"phase\":2,\"ready\":%s,"
      "\"frequency_hz\":%lu,\"crystal_hz\":%lu,\"profile\":\"%s\","
      "\"bandwidth_hz\":%lu,\"data_rate_millibaud\":%lu,\"marcstate\":%u,"
      "\"rssi_dbm_x2\":%d,\"carrier_sense\":%s,\"gdo0\":%d,\"gdo2\":%d}\n",
      ready ? "true" : "false", static_cast<unsigned long>(frequencyHz),
      static_cast<unsigned long>(radio_config::kCrystalHz), profile.name,
      static_cast<unsigned long>(radio_config::bandwidthHz(profile)),
      static_cast<unsigned long>(radio_config::dataRateMilliBaud(profile)), value.marcstate,
      radio_config::rssiDbmTimes2(value.rssi), (value.pktstatus & 0x40) ? "true" : "false",
      digitalRead(config::pins::kGdo0), digitalRead(config::pins::kGdo2));
}

void finishCapture(const char* reason) {
  const auto summary = pulses.stop();
  capturing = false;
  const auto idleResult = radio.stopReceive();
  const auto total = pulses.count();
  Serial.printf(
      "{\"type\":\"capture_begin\",\"count\":%u,\"pre_count\":%lu,"
      "\"post_count\":%lu,\"reason\":\"%s\",\"triggered\":%s,"
      "\"overflow\":%s,\"prehistory_clipped\":%s,\"ambiguous_edges\":%lu,"
      "\"zero_durations\":%lu,\"elapsed_us\":%lu,\"tail_us\":%lu,\"tail_level\":%u,"
      "\"rssi_peak_dbm_x2\":%d,\"rssi_trigger_dbm_x2\":%d,\"carrier_samples\":%lu,"
      "\"frequency_hz\":%lu,\"profile\":\"%s\",\"idle_error\":%d}\n",
      static_cast<unsigned>(total), static_cast<unsigned long>(summary.preCount),
      static_cast<unsigned long>(summary.postCount), reason, triggered ? "true" : "false",
      summary.overflow ? "true" : "false", summary.prehistoryClipped ? "true" : "false",
      static_cast<unsigned long>(summary.ambiguousEdges),
      static_cast<unsigned long>(summary.zeroDurations),
      static_cast<unsigned long>(summary.elapsedUs), static_cast<unsigned long>(summary.tailUs),
      summary.tailLevel, peakDbmX2, triggerActualX2, static_cast<unsigned long>(carrierSamples),
      static_cast<unsigned long>(frequencyHz), radio_config::kProfiles[profileIndex].name,
      static_cast<int>(idleResult));
  // Bulk serial output starts only after capture stops; bit 0 is level, remaining bits are microseconds.
  // 捕获已停止，才允许串口大量输出；每个元素低位为电平，其余位为微秒。
  for (std::size_t offset = 0; offset < total; offset += 32) {
    Serial.printf("{\"type\":\"pulses\",\"offset\":%u,\"items\":[", static_cast<unsigned>(offset));
    for (std::size_t i = offset; i < total && i < offset + 32; ++i) {
      if (i != offset) Serial.print(',');
      Serial.print(pulses.at(i));
    }
    Serial.println("]}");
    delay(0);
  }
  Serial.printf("{\"type\":\"capture_end\",\"count\":%u}\n", static_cast<unsigned>(total));
  const auto result = radio.startReceive();
  if (result != cc1101::BusError::None) {
    ready = false;
    error(result);
  }
}

void armCapture(uint32_t recordMs, uint32_t awaitMs, int threshold, bool immediate) {
  if (!ready || recordMs < 100 || recordMs > 10000 || awaitMs > 60000 || threshold < -240 ||
      threshold > 0) {
    Serial.println("{\"type\":\"error\",\"message\":\"invalid_capture_settings\"}");
    return;
  }
  durationMs = recordMs;
  waitMs = awaitMs;
  triggerDbmX2 = threshold;
  triggered = immediate;
  peakDbmX2 = triggerActualX2 = -1000;
  carrierSamples = 0;
  // Emit ACK before enabling capture; remain silent on serial while capturing.
  // ACK 在开启采样前输出；采样期间不再输出任何串口内容。
  Serial.printf(
      "{\"type\":\"armed\",\"duration_ms\":%lu,\"wait_ms\":%lu,"
      "\"trigger_dbm_x2\":%d,\"immediate\":%s}\n",
      static_cast<unsigned long>(recordMs), static_cast<unsigned long>(awaitMs), threshold,
      immediate ? "true" : "false");
  Serial.flush();
  pulses.arm(!immediate);
  armedMs = triggeredMs = millis();
  capturing = true;
}

void execute(char* line) {
  if (std::strcmp(line, "status") == 0) {
    status();
    return;
  }
  unsigned long recordMs = 0, awaitMs = 0, requestedKhz = 0;
  int threshold = 0;
  char profileName[16]{}, extra = 0;
  if (std::sscanf(line, "capture %lu %c", &recordMs, &extra) == 1) {
    armCapture(recordMs, 0, -140, true);
  } else if (std::sscanf(line, "arm %lu %lu %d %c", &recordMs, &awaitMs, &threshold, &extra) == 3) {
    armCapture(recordMs, awaitMs, threshold, false);
  } else if (std::sscanf(line, "tune %lu %15s %c", &requestedKhz, profileName, &extra) == 2) {
    std::size_t chosen = radio_config::kProfileCount;
    for (std::size_t i = 0; i < radio_config::kProfileCount; ++i) {
      if (std::strcmp(profileName, radio_config::kProfiles[i].name) == 0) chosen = i;
    }
    if (requestedKhz < radio_config::kMinFrequencyHz / 1000 ||
        requestedKhz > radio_config::kMaxFrequencyHz / 1000 ||
        chosen == radio_config::kProfileCount) {
      Serial.println("{\"type\":\"error\",\"message\":\"invalid_receive_settings\"}");
      return;
    }
    const auto result = radio.configureReceive(requestedKhz * 1000, chosen);
    ready = result == cc1101::BusError::None;
    if (!ready) {
      error(result);
      return;
    }
    frequencyHz = requestedKhz * 1000;
    profileIndex = chosen;
    status();
  } else {
    Serial.println(
        "{\"type\":\"error\",\"message\":\"commands: status; capture MS; arm MS WAIT_MS RSSI_X2; "
        "tune KHZ PROFILE\"}");
  }
}
}  // namespace

void setup() {
  Serial.begin(config::kSerialBaud);
  delay(800);
  Serial.println("=== Phase 2：CC1101 原始接收探测，射频参数尚未确认 ===");
  auto result = radio.begin();
  if (result != cc1101::BusError::None) {
    error(result);
    return;
  }
  cc1101::Snapshot identity;
  result = radio.readSnapshot(identity);
  if (result != cc1101::BusError::None) {
    error(result);
    return;
  }
  Serial.printf("{\"type\":\"identity\",\"partnum\":%u,\"version\":%u,\"marcstate\":%u}\n",
                identity.partnum, identity.version, identity.marcstate);
  if (cc1101::validate(identity) != cc1101::IdentityError::None) {
    Serial.println("{\"type\":\"error\",\"message\":\"identity_check_failed\"}");
    return;
  }
  const auto captureError = pulses.begin();
  if (captureError != ESP_OK) {
    Serial.printf("{\"type\":\"error\",\"capture_error\":%d}\n", captureError);
    return;
  }
  result = radio.configureReceive(frequencyHz, profileIndex);
  if (result != cc1101::BusError::None) {
    error(result);
    return;
  }
  ready = true;
  status();
}

void loop() {
  if (capturing) {
    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastRssiMs) >= 2) {
      lastRssiMs = now;
      uint8_t raw = 0;
      const auto result = radio.readRssi(raw);
      if (result != cc1101::BusError::None) {
        finishCapture("spi_error");
        error(result);
        return;
      }
      const int current = radio_config::rssiDbmTimes2(raw);
      if (current > peakDbmX2) peakDbmX2 = current;
      if (digitalRead(config::pins::kGdo2)) ++carrierSamples;
      if (!triggered && current >= triggerDbmX2) {
        triggered = true;
        triggeredMs = now;
        triggerActualX2 = current;
        pulses.trigger();
      }
    }
    if (pulses.state() == capture::State::Full)
      finishCapture("buffer_full");
    else if (triggered && static_cast<uint32_t>(now - triggeredMs) >= durationMs)
      finishCapture("duration");
    else if (!triggered && static_cast<uint32_t>(now - armedMs) >= waitMs)
      finishCapture("trigger_timeout");
    delay(1);
    return;
  }
  while (Serial.available()) {
    const char ch = static_cast<char>(Serial.read());
    if (ch == '\n') {
      if (commandTooLong)
        Serial.println("{\"type\":\"error\",\"message\":\"command_too_long\"}");
      else if (commandSize > 0) {
        command[commandSize] = '\0';
        execute(command);
      }
      commandSize = 0;
      commandTooLong = false;
      if (capturing) break;
    } else if (ch != '\r') {
      if (commandSize + 1 < sizeof(command))
        command[commandSize++] = ch;
      else
        commandTooLong = true;
    }
  }
  delay(1);
}
