#pragma once

#include "flagset.hpp"
#include "keyset.hpp"
#include "logging.hpp"

namespace fujinami {
class KeysetProperty final {
 public:
  bool is_mapped() const noexcept { return flags_ & Flag::MAPPED; }

  bool is_node() const noexcept { return flags_ & Flag::NODE; }

  bool is_leaf() const noexcept { return !(flags_ & Flag::NODE); }

  bool is_combinable(Key key) const noexcept { return combinable_keyset_[key]; }

  const Keyset& combinable_keyset() const noexcept {
    return combinable_keyset_;
  }

  const Keyset& trigger_keyset() const noexcept { return trigger_keyset_; }

  const Keyset& modifier_keyset() const noexcept { return modifier_keyset_; }

  void make_node(const Keyset& combinable_keyset) noexcept {
    combinable_keyset_ += combinable_keyset;
    flags_.set(Flag::NODE, !!combinable_keyset_);
  }

  void make_mapped(const Keyset& trigger_keyset,
                   const Keyset& modifier_keyset) noexcept {
    flags_ += Flag::MAPPED;
    trigger_keyset_ = trigger_keyset;
    modifier_keyset_ = modifier_keyset;
  }

  FUJINAMI_LOGGING_STRUCT(
      KeysetProperty,
      (("flags", flags_))(("combinable_keyset", combinable_keyset_))(
          ("trigger_keyset", trigger_keyset_))(("modifier_keyset",
                                                modifier_keyset_)));

 private:
  enum class Flag : uint8_t {
    MAPPED = 1 << 0,  // マッピングが存在する
    NODE = 1 << 1,    // 組み合わせ可能なキーが存在する
  };
  using Flags = Flagset<Flag>;
  FUJINAMI_FLAGSET_OPERATORS(friend, Flags);
  FUJINAMI_LOGGING_ENUM(friend, Flag, (MAPPED)(NODE));

  Flags flags_{};
  Keyset combinable_keyset_;  // 組み合わせ可能なキーのセット
  Keyset trigger_keyset_;     // トリガーキーのセット
  Keyset modifier_keyset_;    // 修飾キーのセット
};
}  // namespace fujinami
