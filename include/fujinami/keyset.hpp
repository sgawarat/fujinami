#pragma once

#include <bitset>
#include <gsl/gsl>
#include "key.hpp"
#include "logging.hpp"

namespace fujinami {
class Keyset {
 public:
  Keyset() = default;

  Keyset(Key key) noexcept {
    if (key != Key::UNKNOWN) {
      pressed_keys_.set(static_cast<std::underlying_type_t<Key>>(key));
    }
  }

  Keyset(gsl::span<const Key> keys) noexcept {
    for (auto&& key : keys) {
      if (key != Key::UNKNOWN) {
        pressed_keys_.set(static_cast<std::underlying_type_t<Key>>(key));
      }
    }
  }

  Keyset(std::initializer_list<const Key> list) noexcept
      : Keyset(gsl::span<const Key>{list.begin(), list.end()}) {}

  bool operator==(const Keyset& other) const noexcept {
    return pressed_keys_ == other.pressed_keys_;
  }

  bool operator!=(const Keyset& other) const noexcept {
    return pressed_keys_ != other.pressed_keys_;
  }

  Keyset& operator+=(Key key) noexcept {
    if (key != Key::UNKNOWN) {
      pressed_keys_.set(static_cast<std::underlying_type_t<Key>>(key));
    }
    return *this;
  }

  Keyset& operator+=(const Keyset& other) noexcept {
    pressed_keys_ |= other.pressed_keys_;
    return *this;
  }

  Keyset& operator-=(Key key) noexcept {
    if (key != Key::UNKNOWN) {
      pressed_keys_.reset(static_cast<std::underlying_type_t<Key>>(key));
    }
    return *this;
  }

  Keyset& operator-=(const Keyset& other) noexcept {
    pressed_keys_ &= ~other.pressed_keys_;
    return *this;
  }

  Keyset operator+(Key key) const noexcept { return Keyset(*this) += key; }

  Keyset operator+(const Keyset& other) const noexcept {
    return Keyset(*this) += other;
  }

  Keyset operator-(Key key) const noexcept { return Keyset(*this) -= key; }

  Keyset operator-(const Keyset& other) const noexcept {
    return Keyset(*this) -= other;
  }

  bool operator[](Key key) const noexcept {
    if (key == Key::UNKNOWN) return false;
    return pressed_keys_.test(static_cast<std::underlying_type_t<Key>>(key));
  }

  explicit operator bool() const noexcept { return pressed_keys_.any(); }

  Keyset& reset() noexcept {
    pressed_keys_.reset();
    return *this;
  }

  bool contains(const Keyset& keyset) const noexcept {
    return (pressed_keys_ & keyset.pressed_keys_) == keyset.pressed_keys_;
  }

  size_t count() const noexcept { return pressed_keys_.count(); }

  friend Keyset operator+(Key key, const Keyset& keyset) noexcept {
    return keyset + key;
  }

  friend Keyset operator-(Key key, const Keyset& keyset) noexcept {
    return keyset - key;
  }

  friend size_t hash_value(const Keyset& keyset) noexcept {
    return std::hash<decltype(keyset.pressed_keys_)>{}(keyset.pressed_keys_);
  }

  FUJINAMI_LOGGING_DEFINE_PRINT(friend, Keyset, keyset, ({
                                  logging::Separator sep;
                                  os << '[';
                                  for (size_t i = 0; i < KEY_COUNT; ++i) {
                                    if (keyset.pressed_keys_[i])
                                      os << sep << static_cast<Key>(i);
                                  }
                                  os << ']';
                                }));

 private:
  std::bitset<KEY_COUNT> pressed_keys_;
};
}  // namespace fujinami

namespace std {
template <>
struct hash<::fujinami::Keyset>
    : std::unary_function<const ::fujinami::Keyset&, size_t> {
  size_t operator()(const ::fujinami::Keyset& keyset) const noexcept {
    return hash_value(keyset);
  }
};
}  // namespace std
