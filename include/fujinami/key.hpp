#pragma once

#include "flagset.hpp"
#include "platform.hpp"
#if defined(FUJINAMI_PLATFORM_WIN32)
#include <fujinami_win32/key.hpp>
#elif defined(FUJINAMI_PLATFORM_LINUX)
#include <fujinami_linux/key.hpp>
#endif

namespace fujinami {
enum class Modifier : uint16_t {
  SHIFT_LEFT = 1 << 0,
  SHIFT_RIGHT = 1 << 1,
  CONTROL_LEFT = 1 << 2,
  CONTROL_RIGHT = 1 << 3,
  ALT_LEFT = 1 << 4,
  ALT_RIGHT = 1 << 5,
  OS_LEFT = 1 << 6,
  OS_RIGHT = 1 << 7,
  ALL = (1 << 8) - 1,
};
using Modifiers = Flagset<Modifier>;
FUJINAMI_FLAGSET_OPERATORS(inline, Modifiers);
FUJINAMI_LOGGING_FLAGSET(inline, Modifier, Modifiers,
                         (SHIFT_LEFT)(SHIFT_RIGHT)(CONTROL_LEFT)(CONTROL_RIGHT)(
                             ALT_LEFT)(ALT_RIGHT)(OS_LEFT)(OS_RIGHT));
}  // namespace fujinami
