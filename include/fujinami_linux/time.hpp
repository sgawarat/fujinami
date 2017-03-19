#pragma once

#include <chrono>
#include <sys/time.h>
#include <fujinami/logging.hpp>

namespace fujinami {
constexpr std::chrono::microseconds to_duration(const timeval& tv) noexcept {
  return std::chrono::seconds(tv.tv_sec) +
         std::chrono::microseconds(tv.tv_usec);
}

struct LinuxClock final {
  using rep = int64_t;
  using period = std::micro;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<LinuxClock>;
  static constexpr bool is_steady = false;
  static time_point now() noexcept {
    timeval tv;
    gettimeofday(&tv, nullptr);
    return time_point(to_duration(tv));
  }
};
using Clock = LinuxClock;

namespace logging {
inline std::ostream& operator<<(std::ostream& os, const timeval& tv) {
  os << to_duration(tv);
  return os;
}
}  // namespace logging
}  // namespace fujinami
