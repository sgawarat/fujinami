#include <fujinami/logging.hpp>
#include <fujinami/platform/platform.hpp>

namespace fujinami {
namespace logging {
namespace {
#if !defined(NDEBUG) || defined(DEVEL)
#ifdef FUJINAMI_PLATFORM_WIN32
FILE* console_fp = nullptr;
#endif
#endif
}  // namespace

std::shared_ptr<spdlog::logger> Logger::logger_;
thread_local std::vector<size_t> Logger::offsets_;
thread_local std::string Logger::buffer_;

void Logger::init() {
#if !defined(NDEBUG) || defined(DEVEL)
#ifdef FUJINAMI_PLATFORM_WIN32
  AllocConsole();
  freopen_s(&console_fp, "CONOUT$", "w", stdout);
  SetWindowPos(GetConsoleWindow(), HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE);
#endif
  logger_ = spdlog::stdout_logger_mt("logger");
  logger_->set_level(spdlog::level::trace);
#else
#ifdef FUJINAMI_PLATFORM_WIN32
  logger_ = spdlog::rotating_logger_mt("logger", "log", 5 * 1024 * 1024, 8);
#else
  logger_ = spdlog::stdout_logger_mt("logger");
#endif
#ifndef DEVEL
  logger_->set_level(spdlog::level::info);
#else
  logger_->set_level(spdlog::level::trace);
#endif
#endif
  logger_->set_pattern("[%C/%m/%d %X](%L) %v");
  logger_->flush_on(spdlog::level::err);
}

void Logger::init_tls(gsl::not_null<gsl::czstring> name) {
  offsets_.reserve(16);
  buffer_.reserve(1024);
  buffer_.push_back('[');
  buffer_ += name.get();
  buffer_.push_back(']');
  buffer_.push_back(' ');
}

void Logger::terminate() {
  logger_.reset();
  spdlog::drop_all();
#if !defined(NDEBUG) || defined(DEVEL)
#ifdef FUJINAMI_PLATFORM_WIN32
  fclose(console_fp);
  FreeConsole();
#endif
#endif
}

void Logger::enter_section(gsl::not_null<gsl::czstring> name) {
  assert(offsets_.size() <= 64);
  if (buffer_.empty()) buffer_ = "[] ";
  buffer_.pop_back();  // remove ' '
  offsets_.push_back(buffer_.size());
  buffer_.back() = '>';  // replace ']' to '>'
  buffer_ += name.get();
  buffer_.push_back(']');
  buffer_.push_back(' ');
  // "[A] " -> "[A>name] "
}

void Logger::leave_section() {
  assert(!offsets_.empty());
  if (offsets_.empty()) return;
  buffer_.erase(buffer_.begin() + offsets_.back(),
                buffer_.end());  // remove "name] "
  buffer_.back() = ']';          // replace '>' to ']'
  buffer_.push_back(' ');
  offsets_.pop_back();
  // "[A>name] " -> "[A] "
}
}  // namespace logging
}  // namespace fujinami
