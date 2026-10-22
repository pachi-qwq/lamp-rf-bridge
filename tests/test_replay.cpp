// SPDX-License-Identifier: MIT
#include <cstdlib>
#include <iostream>
#include "remote_frames.h"

int main() {
  unsigned checks = 0;
  const auto check = [&checks](bool condition) {
    ++checks;
    if (!condition) {
      std::cerr << "FAIL check " << checks << '\n';
      std::exit(1);
    }
  };
  replay::Plan plan;
  check(replay::buildPlan(replay::kFrame_power, 50, 3, plan));
  check(plan.count == 152 && plan.durationUs < 200000);
  check(plan.pulses[0].level == 0 && plan.pulses[151].level == 0);
  uint32_t total = 0;
  for (std::size_t i = 0; i < plan.count; ++i) total += plan.pulses[i].durationUs;
  check(total == plan.durationUs);
  for (unsigned n = 0; n < 3; ++n) {
    for (unsigned i = 0; i < 50; ++i) {
      check(plan.pulses[1 + n * 50 + i].durationUs == replay::kFrame_power[i].durationUs &&
            plan.pulses[1 + n * 50 + i].level == replay::kFrame_power[i].level);
    }
  }
  check(!replay::buildPlan(nullptr, 50, 3, plan) && plan.count == 0);
  check(!replay::buildPlan(replay::kFrame_power, 49, 3, plan));
  check(!replay::buildPlan(replay::kFrame_power, 50, 0, plan));
  check(!replay::buildPlan(replay::kFrame_power, 50, 4, plan));
  replay::Pulse invalid[50];
  for (unsigned i = 0; i < 50; ++i) invalid[i] = replay::kFrame_power[i];
  invalid[0].level = 0;
  check(!replay::buildPlan(invalid, 50, 3, plan));
  invalid[0] = replay::kFrame_power[0];
  invalid[3].durationUs = 0;
  check(!replay::buildPlan(invalid, 50, 3, plan));
  invalid[3] = replay::kFrame_power[3];
  invalid[49].durationUs = 40000;
  check(!replay::buildPlan(invalid, 50, 3, plan));
  for (unsigned i = 0; i < 49; ++i) invalid[i].durationUs = 1600;
  invalid[49].durationUs = 20000;
  check(!replay::buildPlan(invalid, 50, 3, plan) && plan.count == 0);
  for (const auto& frame : replay::kFrames) {
    check(replay::findFrame(frame.name) == &frame);
    check(replay::buildPlan(frame.pulses, 50, frame.repeats, plan));
    check(plan.count == 2 + 50U * frame.repeats && plan.durationUs <= 200000);
  }
  check(replay::findFrame("unknown") == nullptr);
  check(replay::findFrame("power extra") == nullptr);
  replay::Cooldown cooldown;
  check(cooldown.available(0));
  cooldown.attempted(0xFFFFFFF0U);
  check(!cooldown.available(0x00000000U));
  check(!cooldown.available(0x00000003U));
  check(cooldown.available(0x00000004U));
  std::cout << "PASS: " << checks << " replay checks (pure logic, no hardware simulation)\n";
}
