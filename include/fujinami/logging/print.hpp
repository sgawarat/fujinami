#pragma once

#include <array>
#include <chrono>
#include <vector>
#include <gsl/gsl>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

namespace fujinami {
namespace logging {
class Separator final {
 public:
  Separator(char c = ',') : c_(c) {}

  friend std::ostream& operator<<(std::ostream& os, Separator& sep) {
    // 初回以降に限り区切り文字をログに流す
    if (!sep.b_) {
      sep.b_ = true;
    } else {
      os << sep.c_;
    }
    return os;
  }

 private:
  char c_ = ',';
  bool b_ = false;
};

template <typename T, size_t N>
inline std::ostream& operator<<(std::ostream& os, const std::array<T, N>& ary) {
  Separator sep;
  os << '[';
  for (const T& v : ary) {
    os << sep << v;
  }
  os << ']';
  return os;
}

template <typename T, typename A>
inline std::ostream& operator<<(std::ostream& os,
                                const std::vector<T, A>& vec) {
  Separator sep;
  os << '[';
  for (const T& v : vec) {
    os << sep << v;
  }
  os << ']';
  return os;
}

template <typename Rep>
inline std::ostream& operator<<(std::ostream& os,
                                const std::chrono::duration<Rep, std::milli>& msec) {
  os << msec.count() << "ms";
  return os;
}

template <typename Rep>
inline std::ostream& operator<<(std::ostream& os,
                                const std::chrono::duration<Rep, std::micro>& usec) {
  os << usec.count() << "us";
  return os;
}

template <typename Rep>
inline std::ostream& operator<<(std::ostream& os,
                                const std::chrono::duration<Rep, std::nano>& nsec) {
  os << nsec.count() << "ns";
  return os;
}

template <typename Clock, typename Duration>
inline std::ostream& operator<<(std::ostream& os,
                                const std::chrono::time_point<Clock, Duration>& tp) {
  os << tp.time_since_epoch();
  return os;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const gsl::span<T>& span) {
  Separator sep;
  os << '[';
  for (const T& v : span) {
    os << sep << v;
  }
  os << ']';
  return os;
}
}  // namespace logging
}  // namespace fujinami
