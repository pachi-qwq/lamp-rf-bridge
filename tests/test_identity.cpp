// SPDX-License-Identifier: MIT
// Test numeric validation and streak counting only; do not simulate SPI, GPIO or hardware responses.
// 仅测试纯数值校验与连续计数；不模拟 SPI、GPIO 或任何真实硬件响应。
#include <cstdio>

#include "cc1101_identity.h"

namespace {
int checks = 0;
int errors = 0;

void check(bool condition, const char* name) {
  ++checks;
  if (!condition) {
    ++errors;
    std::printf("FAIL: %s\n", name);
  }
}
}  // namespace

int main() {
  using namespace cc1101;
  check(registers::statusCommand(registers::kPartnum) == 0xF0, "PARTNUM command");
  check(registers::statusCommand(registers::kVersion) == 0xF1, "VERSION command");
  check(registers::statusCommand(registers::kMarcstate) == 0xF5, "MARCSTATE command");

  Snapshot value{};
  check(validate(value) == IdentityError::AllZero, "reject all zero");
  value.partnum = value.version = 0xFF;
  check(validate(value) == IdentityError::AllOnes, "reject all ones");
  value = {0x00, 0x14, 0x01, 0x00, 0x00, 0x00};
  check(validate(value) == IdentityError::None, "accept specified identity");
  value.version = 0x04;
  check(validate(value) == IdentityError::None, "accept documented older silicon");
  value.version = 0x17;
  check(validate(value) == IdentityError::UnknownVersion, "reject unknown silicon");
  value.version = 0xFF;
  check(validate(value) == IdentityError::UnknownVersion, "reject invalid version");
  value.version = 0x14;
  value.partnum = 0x01;
  check(validate(value) == IdentityError::UnexpectedPart, "reject other part");
  value.partnum = 0;
  value.versionStatus = 0x80;
  check(validate(value) == IdentityError::NotReady, "reject not ready");
  value.versionStatus = 0;
  value.marcstate = 0x13;
  check(validate(value) == IdentityError::NotIdle, "reject TX state");
  value.marcstate = 0x81;
  check(validate(value) == IdentityError::NotIdle, "reject reserved MARCSTATE bits");
  value.marcstate = 1;
  value.partStatus = 0x10;
  check(validate(value) == IdentityError::NotIdle, "reject inconsistent status state");
  value.partStatus = 0;

  StabilityTracker tracker(10);
  for (int i = 0; i < 9; ++i) {
    check(!tracker.observe(value), "insufficient continuous observations");
  }
  check(tracker.observe(value) && tracker.count() == 10, "threshold reached");
  check(tracker.observe(value) && tracker.count() == 10, "counter saturates");
  value.version = 0x04;
  check(!tracker.observe(value) && tracker.count() == 1, "identity change restarts count");
  value.version = 0;
  check(!tracker.observe(value) && tracker.count() == 0, "invalid sample breaks streak");
  value.version = 0x14;
  tracker.observe(value);
  tracker.clear();
  check(!tracker.passed() && tracker.count() == 0, "transport failure clears streak");
  StabilityTracker disabled(0);
  check(!disabled.observe(value), "zero threshold never passes");

  std::printf("%d checks, %d failures (pure logic only; no hardware test).\n", checks, errors);
  return errors == 0 ? 0 : 1;
}
