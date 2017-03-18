#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <fujinami/logging.hpp>
#include "event.hpp"
#include "engine.hpp"

namespace fujinami {
namespace mapping {
class Context final {
 public:
  Context(Engine& engine) {}

  bool send_event(const AnyEvent& event) noexcept {
    if (is_closed_) return false;
    std::lock_guard<std::mutex> lck(queue_mtx_);
    event_queue_.push_back(event);
    queue_cv_.notify_one();
    return true;
  }

  bool send_press(const Keyset& active_keyset,
                  std::shared_ptr<const KeyboardLayout> next_layout) noexcept {
    return send_event(KeyPressEvent(active_keyset)) &&
           send_event(LayoutEvent(std::move(next_layout)));
  }

  bool send_repeat(const Keyset& active_keyset) noexcept {
    return send_event(KeyRepeatEvent(active_keyset));
  }

  bool send_release(const Keyset& active_keyset) noexcept {
    return send_event(KeyReleaseEvent(active_keyset));
  }

  bool send_layout(std::shared_ptr<const KeyboardLayout> layout) noexcept {
    return send_event(LayoutEvent(std::move(layout)));
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
}  // namespace mapping
}  // namespace fujinami
