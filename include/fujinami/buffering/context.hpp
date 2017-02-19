#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include "../logging.hpp"
#include "event.hpp"
#include "engine.hpp"

namespace fujinami {
namespace buffering {
class Context final {
 public:
  Context() = delete;
  Context(const Context&) = delete;
  Context(Context&&) = delete;
  Context& operator=(const Context&) = delete;
  Context& operator=(Context&&) = delete;

  Context(Engine& engine) {}

  bool send_event(const AnyEvent& event) noexcept {
    if (is_closed_) return false;
    std::lock_guard<std::mutex> lck(queue_mtx_);
    event_queue_.push_back(event);
    queue_cv_.notify_one();
    return true;
  }

  bool receive_event(const Clock::time_point& timeout_tp,
                     AnyEvent& event) noexcept {
    if (is_closed_) return false;
    std::unique_lock<std::mutex> lck(queue_mtx_);
    if (event_queue_.empty()) {
      queue_cv_.wait_until(lck, timeout_tp, [&]() noexcept {
        return is_closed_ || !event_queue_.empty();
      });
      if (is_closed_ || event_queue_.empty()) return false;
    }
    event = std::move(event_queue_.front());
    event_queue_.pop_front();
    return true;
  }

  bool receive_event(AnyEvent& event) noexcept {
    if (is_closed_) return false;
    std::unique_lock<std::mutex> lck(queue_mtx_);
    if (event_queue_.empty()) {
      queue_cv_.wait(
          lck, [&]() noexcept { return is_closed_ || !event_queue_.empty(); });
      if (is_closed_ || event_queue_.empty()) return false;
    }
    event = std::move(event_queue_.front());
    event_queue_.pop_front();
    return true;
  }

  void reset() noexcept {
    std::lock_guard<std::mutex> lck(queue_mtx_);
    event_queue_.clear();
  }

  void close() noexcept {
    is_closed_ = true;
    std::lock_guard<std::mutex> lck(queue_mtx_);
    queue_cv_.notify_one();
  }

  bool is_closed() const noexcept { return is_closed_; }

 private:
  std::atomic<bool> is_closed_{false};
  std::deque<AnyEvent> event_queue_;
  std::mutex queue_mtx_;
  std::condition_variable queue_cv_;
};
}  // namespace buffering
}  // namespace fujinami
