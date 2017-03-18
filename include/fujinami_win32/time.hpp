#pragma once

#include <Windows.h>
#include <chrono>
#include <fujinami/logging.hpp>

namespace fujinami {
struct Win32Clock final {
  using rep = DWORD;
  using period = std::milli;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<Win32Clock>;
  static constexpr bool is_steady = false;
  static time_point now() noexcept {
    return time_point(duration(GetTickCount()));
  }
};

using Clock = Win32Clock;
}  // namespace fujinami
