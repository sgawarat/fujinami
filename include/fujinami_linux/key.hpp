#pragma once

#include <array>
#include <tuple>
#include <gsl/gsl>
#include <linux/input.h>
#include <fujinami/logging.hpp>

namespace fujinami {
enum class Key : uint8_t {
  UNKNOWN = 0,
};
static constexpr size_t KEY_COUNT = 256;
FUJINAMI_LOGGING_DEFINE_PRINT(
    inline, Key, key, (os << static_cast<uint32_t>(key);));

inline Key to_key(__u16 code) noexcept {
  if (code >= KEY_COUNT) return Key::UNKNOWN;
  return static_cast<Key>(code);
}

inline uint16_t to_keycode(Key key) noexcept { return static_cast<__u16>(key); }
}  // namespace fujinami
