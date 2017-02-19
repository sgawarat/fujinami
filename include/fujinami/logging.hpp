#pragma once

// FUJINAMI_LOGGING_SUPRESS: 内部データの出力を隠す
// FUJINAMI_LOGGING_TRACE_ON: トレースのログを出力する
// FUJINAMI_LOGGING_DEBUG_ON: デバッグのログを出力する

#if !defined(DEVEL) && defined(NDEBUG)
#define FUJINAMI_LOGGING_SUPRESS
#else
#define FUJINAMI_LOGGING_TRACE_ON
#define FUJINAMI_LOGGING_DEBUG_ON
#endif

#include <gsl/gsl>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include "logging/print.hpp"
#include "logging/macro.hpp"

namespace fujinami {
namespace logging {
template <typename T, typename = void>
class Printable final {
 public:
  constexpr explicit Printable(const T& value) noexcept : value_(value) {}
  friend std::ostream& operator<<(std::ostream& os, const Printable& self) {
    using ::fujinami::logging::operator<<;
    os << self.value_;
    return os;
  }

 protected:
  const T& value_;
};

template <>
class Printable<const char*, void> final {
 public:
  constexpr explicit Printable(const char* value) noexcept : value_(value) {}
  friend std::ostream& operator<<(std::ostream& os, const Printable& self) {
    using fujinami::logging::operator<<;
    if (self.value_) {
      os << self.value_;
    } else {
      os << "null";
    }
    return os;
  }

 protected:
  const char* value_;
};
;

template <typename T>
class Printable<const T*, std::enable_if_t<!std::is_same<T, char>::value>>
    final {
 public:
  constexpr explicit Printable(const T* value) noexcept : value_(value) {}
  friend std::ostream& operator<<(std::ostream& os, const Printable& self) {
    using fujinami::logging::operator<<;
    if (self.value_) {
      os << *self.value_;
    } else {
      os << "null";
    }
    return os;
  }

 protected:
  const T* value_;
};

class Logger final {
 public:
  static void init();

  static void init_tls(gsl::not_null<gsl::czstring> name);

  static void terminate();

  static void enter_section(gsl::not_null<gsl::czstring> name);

  static void leave_section();

  template <typename... Args>
  static void trace(gsl::not_null<gsl::czstring> msg, const Args&... args) {
    log(spdlog::level::trace, msg, args...);
  }

  template <typename... Args>
  static void debug(gsl::not_null<gsl::czstring> msg, const Args&... args) {
    log(spdlog::level::debug, msg, args...);
  }

  template <typename... Args>
  static void info(gsl::not_null<gsl::czstring> msg, const Args&... args) {
    log(spdlog::level::info, msg, args...);
  }

  template <typename... Args>
  static void warn(gsl::not_null<gsl::czstring> msg, const Args&... args) {
    log(spdlog::level::warn, msg, args...);
  }

  template <typename... Args>
  static void error(gsl::not_null<gsl::czstring> msg, const Args&... args) {
    log(spdlog::level::err, msg, args...);
  }

  template <typename... Args>
  static void critical(gsl::not_null<gsl::czstring> msg, const Args&... args) {
    log(spdlog::level::critical, msg, args...);
  }

 private:
  Logger() = delete;
  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  ~Logger() = delete;
  Logger& operator=(const Logger&) = delete;
  Logger& operator=(Logger&&) = delete;

  template <typename... Args>
  static void log(spdlog::level::level_enum level,
                  gsl::not_null<gsl::czstring> msg, const Args&... args) {
    if (!logger_) return;
    const size_t section_index = buffer_.size();
    buffer_ += msg.get();
    logger_->log(level, buffer_.c_str(), Printable<Args>(args)...);
    buffer_.erase(buffer_.begin() + section_index, buffer_.end());
  }

  static std::shared_ptr<spdlog::logger> logger_;
  static thread_local std::string buffer_;
  static thread_local std::vector<size_t> offsets_;
};

class ScopedSection final {
 public:
  ScopedSection(gsl::not_null<gsl::czstring> name) {
    Logger::enter_section(name);
  }

  ~ScopedSection() noexcept { Logger::leave_section(); }

  ScopedSection() = delete;
  ScopedSection(const ScopedSection&) = delete;
  ScopedSection(ScopedSection&&) = delete;
  ScopedSection& operator=(const ScopedSection&) = delete;
  ScopedSection& operator=(ScopedSection&&) = delete;
};
}  // namespace logging
}  // namespace fujinami
