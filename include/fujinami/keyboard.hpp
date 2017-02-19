#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <gsl/gsl>
#include "buffering/context.hpp"
#include "buffering/engine.hpp"
#include "mapping/context.hpp"
#include "mapping/engine.hpp"

namespace fujinami {
class Keyboard final {
 public:
  Keyboard(const Keyboard&) = delete;
  Keyboard(Keyboard&&) = delete;
  Keyboard& operator=(const Keyboard&) = delete;
  Keyboard& operator=(Keyboard&&) = delete;

  Keyboard();

  ~Keyboard() noexcept;

  bool open();

  void close() noexcept;

  bool send_event(const buffering::AnyEvent& event) noexcept;

 private:
  std::atomic<bool> is_closed_{true};

  std::thread b_thread_;
  buffering::Engine b_engine_;
  buffering::Context b_context_;

  std::thread m_thread_;
  mapping::Engine m_engine_;
  mapping::Context m_context_;
};
}  // namespace fujinami
