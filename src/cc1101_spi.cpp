// SPDX-License-Identifier: MIT
#include "cc1101_spi.h"

#include <Arduino.h>

#include "config.h"
#include "replay_config.h"

namespace cc1101 {
bool SpiDevice::waitReady() const {
  const uint32_t started = micros();
  while (digitalRead(config::pins::kMiso) != LOW) {
    // Unsigned elapsed time tolerates micros() wrap; a disconnected module must time out.
    // 无符号差值允许 micros() 回绕。断线不能让固件永久卡死。
    if (static_cast<uint32_t>(micros() - started) >= config::kReadyTimeoutUs) {
      return false;
    }
    delayMicroseconds(5);
  }
  return true;
}

BusError SpiDevice::begin() {
  if (initialized_) {
    spi_.end();
  }
  initialized_ = false;
  transmitConfigured_ = false;
  lastCommand_ = registers::kSres;

  // After reset GDO0 defaults to a divided crystal clock; keep both ESP32 GDO pins as inputs.
  // CC1101 复位后 GDO0 默认输出晶振分频时钟；ESP32 两个 GDO 均保持输入。
  pinMode(config::pins::kGdo0, INPUT);
  pinMode(config::pins::kGdo2, INPUT);
  digitalWrite(config::pins::kCsn, HIGH);
  pinMode(config::pins::kCsn, OUTPUT);
  digitalWrite(config::pins::kSck, HIGH);
  pinMode(config::pins::kSck, OUTPUT);
  digitalWrite(config::pins::kMosi, LOW);
  pinMode(config::pins::kMosi, OUTPUT);
  // A weak pull-up makes a disconnected SO time out; it does not replace power/wiring checks.
  // 弱上拉使未连接的 SO 更容易表现为超时，不能替代电源/接线检查。
  pinMode(config::pins::kMiso, INPUT_PULLUP);

  // TI section 19.1.2: SCLK=1, SI=0; pulse CSn low/high and hold high for at least 40 us.
  // TI 19.1.2：SCLK=1、SI=0，CSn 低/高，保持高至少 40 us。
  digitalWrite(config::pins::kCsn, LOW);
  delayMicroseconds(config::kResetPulseUs);
  digitalWrite(config::pins::kCsn, HIGH);
  delayMicroseconds(config::kResetHighUs);
  digitalWrite(config::pins::kCsn, LOW);
  if (!waitReady()) {
    digitalWrite(config::pins::kCsn, HIGH);
    return BusError::ReadyTimeout;
  }

  // Bit-bang only the first SRES to preserve the manual-reset pin levels above.
  // 仅首次 SRES 用 GPIO 移出，以严格保留上述手动复位前置电平。
  // All subsequent register accesses use ESP32 hardware SPI, mode 0, MSB first.
  // 随后的寄存器访问全部使用 ESP32 硬件 SPI、模式 0、MSB first。
  digitalWrite(config::pins::kSck, LOW);
  for (int bit = 7; bit >= 0; --bit) {
    digitalWrite(config::pins::kMosi, (registers::kSres >> bit) & 1U);
    delayMicroseconds(1);
    digitalWrite(config::pins::kSck, HIGH);
    delayMicroseconds(1);
    digitalWrite(config::pins::kSck, LOW);
  }
  // The last bit is zero; allow SO propagation time before checking reset readiness.
  // 最后一位为 0，SO 可能需要短暂传播时间才反映复位期间的忙状态。
  delayMicroseconds(1);
  const bool resetDone = waitReady();
  digitalWrite(config::pins::kCsn, HIGH);
  delayMicroseconds(1);
  if (!resetDone) {
    return BusError::ResetTimeout;
  }

  spi_.begin(config::pins::kSck, config::pins::kMiso, config::pins::kMosi, config::pins::kCsn);
  // Reapply the SO pull-up after spi.begin().
  // spi.begin() 后再次明确设置 SO 弱上拉。
  pinMode(config::pins::kMiso, INPUT_PULLUP);
  initialized_ = true;
  return BusError::None;
}

void SpiDevice::endAccess() {
  delayMicroseconds(1);
  digitalWrite(config::pins::kCsn, HIGH);
  spi_.endTransaction();
  delayMicroseconds(1);
}

BusError SpiDevice::readStatus(uint8_t address, uint8_t& value, uint8_t& chipStatus) {
  lastCommand_ = registers::statusCommand(address);
  spi_.beginTransaction(SPISettings(config::kSpiClockHz, MSBFIRST, SPI_MODE0));
  digitalWrite(config::pins::kCsn, LOW);
  delayMicroseconds(1);
  if (!waitReady()) {
    endAccess();
    return BusError::ReadyTimeout;
  }
  // The first returned byte is chip status; the second is the register value.
  // 首个返回字节是芯片状态，第二个返回字节才是寄存器内容。
  chipStatus = spi_.transfer(lastCommand_);
  value = spi_.transfer(0x00);
  endAccess();
  return BusError::None;
}

BusError SpiDevice::readSnapshot(Snapshot& result) {
  result = {};
  if (!initialized_) {
    return BusError::NotInitialized;
  }
  // Read status registers with separate commands, not a consecutive burst.
  // 不可用一次“突发读”连续取得多个状态寄存器，必须分别发送命令。
  BusError error = readStatus(registers::kPartnum, result.partnum, result.partStatus);
  if (error != BusError::None) {
    return error;
  }
  error = readStatus(registers::kVersion, result.version, result.versionStatus);
  if (error != BusError::None) {
    return error;
  }
  return readStatus(registers::kMarcstate, result.marcstate, result.marcStatus);
}
}  // namespace cc1101

namespace cc1101 {
BusError SpiDevice::select(uint8_t command) {
  if (!initialized_) return BusError::NotInitialized;
  lastCommand_ = command;
  spi_.beginTransaction(SPISettings(config::kSpiClockHz, MSBFIRST, SPI_MODE0));
  digitalWrite(config::pins::kCsn, LOW);
  delayMicroseconds(1);
  if (!waitReady()) {
    endAccess();
    return BusError::ReadyTimeout;
  }
  return BusError::None;
}

BusError SpiDevice::readConfig(uint8_t address, uint8_t& value) {
  auto error = select(address | 0x80);
  if (error != BusError::None) return error;
  const uint8_t chip = spi_.transfer(lastCommand_);
  value = spi_.transfer(0);
  endAccess();
  return (chip & 0x80) ? BusError::ChipNotReady : BusError::None;
}

BusError SpiDevice::writeVerified(uint8_t address, uint8_t value) {
  auto error = select(address);
  if (error != BusError::None) return error;
  const uint8_t chip = spi_.transfer(address);
  spi_.transfer(value);
  endAccess();
  if (chip & 0x80) return BusError::ChipNotReady;
  uint8_t actual = 0;
  error = readConfig(address, actual);
  if (error != BusError::None) return error;
  return actual == value ? BusError::None : BusError::RegisterMismatch;
}

BusError SpiDevice::strobe(uint8_t command) {
  auto error = select(command);
  if (error != BusError::None) return error;
  const uint8_t chip = spi_.transfer(command);
  endAccess();
  return (chip & 0x80) ? BusError::ChipNotReady : BusError::None;
}

BusError SpiDevice::waitState(uint8_t expected) {
  const uint32_t started = millis();
  uint8_t previous = 0xFF;
  do {
    uint8_t current = 0, chip = 0;
    const auto error = readStatus(registers::kMarcstate, current, chip);
    if (error != BusError::None) return error;
    if (chip & 0x80) return BusError::ChipNotReady;
    // Require two matching dynamic-state reads to reduce transition-time ambiguity.
    // 动态状态寄存器连续两次一致，降低状态变化期间读数不可靠的影响。
    if (current == expected && previous == current) return BusError::None;
    previous = current;
    delay(1);
  } while (static_cast<uint32_t>(millis() - started) < 30);
  return BusError::StateTimeout;
}

BusError SpiDevice::stopReceive() {
  const auto error = strobe(0x36);  // SIDLE
  return error == BusError::None ? waitState(registers::kIdle) : error;
}

BusError SpiDevice::startReceive() {
  const auto error = strobe(0x34);  // SRX
  return error == BusError::None ? waitState(0x0D) : error;
}

BusError SpiDevice::configureOok(uint32_t frequencyHz, std::size_t profileIndex, bool receive) {
  transmitConfigured_ = false;
  if (frequencyHz < radio_config::kMinFrequencyHz || frequencyHz > radio_config::kMaxFrequencyHz ||
      profileIndex >= radio_config::kProfileCount)
    return BusError::InvalidSettings;
  auto error = stopReceive();
  if (error != BusError::None) return error;
  const auto& profile = radio_config::kProfiles[profileIndex];
  const uint32_t word = radio_config::frequencyWord(frequencyHz);
  struct Setting {
    uint8_t address;
    uint8_t value;
  };
  const Setting settings[] = {
      {0x00,
       0x0E},  // IOCFG2: carrier-sense output; ESP32 GPIO27 is an input / IOCFG2：载波检测输出，ESP32 GPIO27 输入
      {0x01,
       0x2E},  // IOCFG1: high impedance when deselected, SPI SO while selected / IOCFG1：未选中时三态；选中后仍为 SPI SO
      {0x02,
       static_cast<uint8_t>(
           receive ? 0x0D
                   : 0x2E)},  // Keep GDO0 high impedance while preparing TX / 发射准备时 GDO0 三态
      {0x03, profile.fifothr},
      {0x07, 0x00},  // PKTCTRL1: no address/packet filtering / PKTCTRL1：不使用地址/包过滤
      {0x08,
       0x30},  // PKTCTRL0: asynchronous serial, no CRC or whitening / PKTCTRL0：异步串行，无 CRC、无白化
      {0x0A, 0x00},  // CHANNR: no channel frequency offset / CHANNR：不叠加频道频偏
      {0x0B, profile.fsctrl1},
      {0x0C, 0x00},  // FSCTRL0: no additional frequency offset / FSCTRL0：无额外频偏
      {0x0D, static_cast<uint8_t>(word >> 16)},
      {0x0E, static_cast<uint8_t>(word >> 8)},
      {0x0F, static_cast<uint8_t>(word)},
      {0x10, profile.mdmcfg4},
      {0x11, profile.mdmcfg3},
      {0x12,
       0x30},  // MDMCFG2: ASK/OOK, Manchester and sync-word detection disabled / MDMCFG2：ASK/OOK，关闭 Manchester 与同步字检测
      {0x13, 0x00},  // MDMCFG1: FEC disabled / MDMCFG1：关闭 FEC
      {0x17, 0x00},  // MCSM1: no automatic TX transition / MCSM1：无自动 TX 切换
      {0x18,
       0x18},  // MCSM0: calibrate automatically when leaving IDLE for RX/TX / MCSM0：从 IDLE 进入 RX/TX 时自动校准
      {0x19,
       radio_config::
           kFoccfg},  // FOCCFG: no frequency-offset compensation for OOK / FOCCFG：OOK 不做频偏补偿
      {0x1B,
       radio_config::
           kAgcCtrl2},  // Initial AGC settings within the DN022 recommendations / DN022 推荐范围内的 AGC 起始组合
      {0x1C, radio_config::kAgcCtrl1},
      {0x1D, radio_config::kAgcCtrl0},
      {0x21, profile.frend1},
      {0x2C, profile.test2},
      {0x2D, profile.test1},
  };
  for (const auto& setting : settings) {
    error = writeVerified(setting.address, setting.value);
    if (error != BusError::None) return error;
  }
  return BusError::None;
}

BusError SpiDevice::configureReceive(uint32_t frequencyHz, std::size_t profileIndex) {
  const auto error = configureOok(frequencyHz, profileIndex, true);
  return error == BusError::None ? startReceive() : error;
}

BusError SpiDevice::readRssi(uint8_t& raw) {
  uint8_t chip = 0;
  if (!initialized_) return BusError::NotInitialized;
  const auto error = readStatus(0x34, raw, chip);
  if (error != BusError::None) return error;
  return (chip & 0x80) ? BusError::ChipNotReady : BusError::None;
}

BusError SpiDevice::readRuntimeStatus(RuntimeStatus& result) {
  if (!initialized_) return BusError::NotInitialized;
  uint8_t chip = 0;
  auto error = readStatus(registers::kMarcstate, result.marcstate, chip);
  if (error != BusError::None) return error;
  error = readRssi(result.rssi);
  if (error != BusError::None) return error;
  return readStatus(0x38, result.pktstatus, chip);
}
}  // namespace cc1101

namespace cc1101 {
BusError SpiDevice::writePaTable() {
  // TI section 10.6: use burst access for PATABLE index 1; CSn high resets the index.
  // TI 10.6：必须突发写才能访问索引 1；CSn 拉高后索引归零。
  const uint8_t table[8] = {0x00, replay_config::kPaOn, 0, 0, 0, 0, 0, 0};
  auto error = select(0x7E);
  if (error != BusError::None) return error;
  uint8_t chip = spi_.transfer(0x7E);
  for (const auto value : table) spi_.transfer(value);
  endAccess();
  if (chip & 0x80) return BusError::ChipNotReady;
  error = select(0xFE);
  if (error != BusError::None) return error;
  chip = spi_.transfer(0xFE);
  bool equal = true;
  for (const auto value : table) {
    const uint8_t actual = spi_.transfer(0);
    if (value != actual) equal = false;
  }
  endAccess();
  if (chip & 0x80) return BusError::ChipNotReady;
  return equal ? BusError::None : BusError::RegisterMismatch;
}

BusError SpiDevice::configureTransmit() {
  // Use the receive-tested nominal frequency; this function issues no STX or RF waveform.
  // 只允许已验证可接收的频率。该函数不发送 STX，不产生射频波形。
  auto error = configureOok(radio_config::kDefaultFrequencyHz, 0, false);
  if (error != BusError::None) return error;
  error = writeVerified(
      0x22,
      0x11);  // FREND0: logic 1 uses PATABLE[1], logic 0 uses [0] / FREND0：逻辑 1 用 PATABLE[1]；0 用 [0]
  if (error == BusError::None) error = writePaTable();
  transmitConfigured_ = error == BusError::None;
  return error;
}

BusError SpiDevice::startTransmit() {
  if (!transmitConfigured_) return BusError::InvalidSettings;
  const auto error = strobe(
      0x35);  // STX: GDO0 is high impedance; ESP32 must already drive the input low / STX：GDO0 已三态，ESP32 必须先提供低电平
  return error == BusError::None ? waitState(0x13) : error;
}

BusError SpiDevice::stopTransmit() {
  return stopReceive();  // SIDLE stops RX/TX; require two consecutive IDLE reads / SIDLE 对 RX/TX 通用，且等待连续两次 IDLE
}
}  // namespace cc1101
