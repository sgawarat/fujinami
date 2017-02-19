#pragma once

#include <cassert>
#include <type_traits>

// フラグ同士の演算を定義するマクロ
#define FUJINAMI_FLAGSET_OPERATORS(specifier, FlagsetT)                      \
  specifier constexpr FlagsetT operator~(FlagsetT::FlagType flag) noexcept { \
    return ~FlagsetT(flag);                                                  \
  }                                                                          \
  specifier constexpr FlagsetT operator|(FlagsetT::FlagType lhs,             \
                                         FlagsetT::FlagType rhs) noexcept {  \
    return FlagsetT(lhs) | rhs;                                              \
  }                                                                          \
  specifier constexpr FlagsetT operator&(FlagsetT::FlagType lhs,             \
                                         FlagsetT::FlagType rhs) noexcept {  \
    return FlagsetT(lhs) & rhs;                                              \
  }                                                                          \
  specifier constexpr FlagsetT operator^(FlagsetT::FlagType lhs,             \
                                         FlagsetT::FlagType rhs) noexcept {  \
    return FlagsetT(lhs) ^ rhs;                                              \
  }                                                                          \
  specifier constexpr FlagsetT operator+(FlagsetT::FlagType lhs,             \
                                         FlagsetT::FlagType rhs) noexcept {  \
    return FlagsetT(lhs) + rhs;                                              \
  }                                                                          \
  specifier constexpr FlagsetT operator-(FlagsetT::FlagType lhs,             \
                                         FlagsetT::FlagType rhs) noexcept {  \
    return FlagsetT(lhs) - rhs;                                              \
  }

namespace fujinami {
template <typename T>
class Flagset final {
 public:
  static_assert(std::is_enum<T>::value, "T must be enum");

  using FlagType = T;
  using ValueType = std::underlying_type_t<T>;

  constexpr Flagset() = default;

  constexpr Flagset(T flag) noexcept : value_(static_cast<ValueType>(flag)) {}

  constexpr operator bool() const noexcept { return !!value_; }

  constexpr bool operator==(Flagset flags) const noexcept {
    return value_ == flags.value_;
  }
  constexpr bool operator!=(Flagset flags) const noexcept {
    return value_ != flags.value_;
  }

  constexpr Flagset operator~() const noexcept {
    return Flagset(value_ ^ ~ValueType(0));
  }

  constexpr Flagset operator|(Flagset flags) const noexcept {
    return Flagset(value_ | flags.value_);
  }

  constexpr Flagset operator&(Flagset flags) const noexcept {
    return Flagset(value_ & flags.value_);
  }

  constexpr Flagset operator^(Flagset flags) const noexcept {
    return Flagset(value_ ^ flags.value_);
  }

  Flagset& operator|=(Flagset flags) noexcept {
    value_ |= flags.value_;
    return *this;
  }

  Flagset& operator&=(Flagset flags) noexcept {
    value_ &= flags.value_;
    return *this;
  }

  Flagset& operator^=(Flagset flags) noexcept {
    value_ ^= flags.value_;
    return *this;
  }

  constexpr Flagset operator+(Flagset flags) const noexcept {
    return Flagset(value_ | flags.value_);
  }

  constexpr Flagset operator-(Flagset flags) const noexcept {
    return Flagset(value_ & ~flags.value_);
  }

  Flagset& operator+=(Flagset flags) noexcept {
    value_ |= flags.value_;
    return *this;
  }

  Flagset& operator-=(Flagset flags) noexcept {
    value_ &= ~flags.value_;
    return *this;
  }

  constexpr Flagset clear() const noexcept { return Flagset{}; }

  Flagset& clear() noexcept {
    value_ = 0;
    return *this;
  }

  constexpr Flagset set(Flagset flags, bool val = true) const noexcept {
    return val ? (*this + flags) : (*this - flags);
  }

  Flagset& set(Flagset flags, bool val = true) noexcept {
    return val ? (*this += flags) : (*this -= flags);
  }

  constexpr Flagset reset(Flagset flags) const noexcept {
    return set(flags, false);
  }

  Flagset& reset(Flagset flags) noexcept { return set(flags, false); }

  constexpr bool is_all(Flagset flags) const noexcept {
    return (value_ & flags.value_) == flags.value_;
  }

  constexpr bool is_all() const noexcept { return !!~value_; }

  constexpr bool is_any(Flagset flags) const noexcept {
    return (value_ & flags.value_) != 0;
  }

  constexpr bool is_any() const noexcept { return !!value_; }

  constexpr ValueType value() const noexcept { return value_; }

  friend constexpr Flagset operator|(T lhs, Flagset rhs) noexcept {
    return Flagset(lhs) | rhs;
  }

  friend constexpr Flagset operator&(T lhs, Flagset rhs) noexcept {
    return Flagset(lhs) & rhs;
  }

  friend constexpr Flagset operator^(T lhs, Flagset rhs) noexcept {
    return Flagset(lhs) ^ rhs;
  }

  friend constexpr Flagset operator+(T lhs, Flagset rhs) noexcept {
    return Flagset(lhs) + rhs;
  }

  friend constexpr Flagset operator-(T lhs, Flagset rhs) noexcept {
    return Flagset(lhs) - rhs;
  }

 private:
  constexpr explicit Flagset(ValueType value) noexcept : value_(value) {}

  ValueType value_ = 0;
};
}  // namespace fujinami
