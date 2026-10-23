// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>

namespace cc1101 {
// Based on TI SWRS061I sections 10, 19.1.2 and 29.3.
// 依据 TI SWRS061I 第 10、19.1.2、29.3 节。
// Set both READ and BURST for status addresses 0x30..0x3D, using separate commands.
// 0x30..0x3D 的状态寄存器必须同时置 READ 和 BURST 位，逐个读取。
namespace registers {
inline constexpr uint8_t kPartnum = 0x30;
inline constexpr uint8_t kVersion = 0x31;
inline constexpr uint8_t kMarcstate = 0x35;
inline constexpr uint8_t kReadStatus = 0xC0;
inline constexpr uint8_t kSres = 0x30;
inline constexpr uint8_t kIdle = 0x01;

/**
 * @brief Add READ|BURST to a status address, not a configuration address.
 * 为状态地址加入 READ|BURST，不适用于配置地址。
 * @param address One status address in 0x30..0x3D. / 0x30..0x3D 内的单个状态地址。
 * @return Command byte; no I/O. / 命令字节；不执行 I/O。
 */
constexpr uint8_t statusCommand(uint8_t address) {
  return static_cast<uint8_t>(address | kReadStatus);
}
}  // namespace registers

/**
 * @brief Raw identity/state registers and the chip-status byte for each command.
 * 原始身份、状态寄存器与每次命令的芯片状态字节。
 */
struct Snapshot {
  uint8_t partnum = 0;
  uint8_t version = 0;
  uint8_t marcstate = 0;
  uint8_t partStatus = 0;
  uint8_t versionStatus = 0;
  uint8_t marcStatus = 0;
};

/**
 * @brief Identity/readiness validation result; None means supported and IDLE.
 * 身份及就绪校验结果；None 表示支持且处于 IDLE。
 */
enum class IdentityError {
  None,
  AllZero,
  AllOnes,
  UnexpectedPart,
  UnknownVersion,
  NotReady,
  NotIdle,
};

// TI documents VERSION=0x14; TI support also confirms the earlier 0x04 revision.
// TI 数据手册列出 VERSION=0x14；TI 官方支持同时确认旧版本 0x04。
// Retain unknown revisions for investigation; a nonzero byte alone does not prove identity.
// 未知版本保留原值供核查，不因“非零”就判定为 CC1101。
/**
 * @brief Validate PARTNUM=0, VERSION=0x04/0x14 and ready/IDLE bits.
 * 校验 PARTNUM=0、VERSION=0x04/0x14 及就绪、IDLE 位。
 * @param sample Complete successful-read snapshot. / 成功读取的完整快照。
 * @return None for valid supported IDLE snapshots, else first error. / 合法且支持的 IDLE 快照返回 None，否则首个错误。
 */
constexpr IdentityError validate(const Snapshot& sample) {
  if (sample.partnum == 0 && sample.version == 0) {
    return IdentityError::AllZero;
  }
  if (sample.partnum == 0xFF && sample.version == 0xFF) {
    return IdentityError::AllOnes;
  }
  if (sample.partnum != 0x00) {
    return IdentityError::UnexpectedPart;
  }
  if (sample.version != 0x04 && sample.version != 0x14) {
    return IdentityError::UnknownVersion;
  }
  if (((sample.partStatus | sample.versionStatus | sample.marcStatus) & 0x80) != 0) {
    return IdentityError::NotReady;
  }
  if (sample.marcstate != registers::kIdle ||
      ((sample.partStatus | sample.versionStatus | sample.marcStatus) & 0x70) != 0) {
    return IdentityError::NotIdle;
  }
  return IdentityError::None;
}

// Count only consecutive valid samples with matching IDs; failure resets the count.
// 稳定性只统计连续、合法、ID 一致的样本；失败会清零。
/**
 * @brief Track consecutive valid snapshots with identical chip IDs; zero requirement never passes.
 * 跟踪 ID 一致的连续合法快照；零次数要求永不通过。
 */
class StabilityTracker {
 public:
  /**
   * @brief Track consecutive valid snapshots with identical chip IDs; zero requirement never passes.
   * 跟踪 ID 一致的连续合法快照；零次数要求永不通过。
   * @param required Required consecutive valid count; zero never passes. / 所需连续合法次数；零永不通过。
   */
  explicit constexpr StabilityTracker(uint32_t required) : required_(required) {}

  /**
   * @brief Reset the streak without changing the required count.
   * 清除连续计数，保留所需次数。
   */
  constexpr void clear() { count_ = 0; }

  /**
   * @brief Consume a complete snapshot; invalid samples reset the streak.
   * 处理完整快照；非法样本清零。
   * @param sample Complete successful-read snapshot. / 成功读取的完整快照。
   * @return True when the required positive streak is reached. / 达到正数要求的连续次数时 true。
   */
  constexpr bool observe(const Snapshot& sample) {
    if (validate(sample) != IdentityError::None) {
      /**
       * @brief Reset the streak without changing the required count.
       * 清除连续计数，保留所需次数。
       */
      clear();
      return false;
    }
    if (count_ == 0 || partnum_ != sample.partnum || version_ != sample.version) {
      partnum_ = sample.partnum;
      version_ = sample.version;
      count_ = 1;
    } else if (count_ < required_) {
      ++count_;
    }
    /**
     * @brief Return true only after a positive required streak is reached.
     * 仅达到正数要求的连续次数后返回 true。
     */
    return passed();
  }

  /**
   * @brief Return the valid streak; a positive required count caps it.
   * 返回当前合法连续次数；正数要求时上限为所需次数。
   */
  constexpr uint32_t count() const { return count_; }
  /**
   * @brief Return true only after a positive required streak is reached.
   * 仅达到正数要求的连续次数后返回 true。
   */
  constexpr bool passed() const { return required_ > 0 && count_ >= required_; }

 private:
  uint32_t required_;
  uint32_t count_ = 0;
  uint8_t partnum_ = 0;
  uint8_t version_ = 0;
};
}  // namespace cc1101
