#pragma once

#include <chrono>
#include <memory>
#include <unordered_map>
#include <vector>
#include "keyboard_layout.hpp"
#include "time.hpp"

namespace fujinami {
class KeyboardConfig final {
 public:
  bool has_timeout_dur() const noexcept { return has_timeout_dur_; }

  const Clock::duration& timeout_dur() const noexcept { return timeout_dur_; }

  std::shared_ptr<const KeyboardLayout> layout(size_t i) const noexcept {
    if (i >= layouts_.size()) return nullptr;
    return layouts_[i];
  }

  size_t layout_count() const noexcept { return layouts_.size(); }

  const std::shared_ptr<const KeyboardLayout>& default_layout() const noexcept {
    return default_layout_;
  }

  const std::shared_ptr<const KeyboardLayout>& default_im_layout() const
      noexcept {
    return default_im_layout_;
  }

  bool auto_layout() const noexcept { return auto_layout_; }

  void reset() {
    has_timeout_dur_ = false;
    timeout_dur_ = Clock::duration::zero();
    layouts_.clear();
    default_layout_ = nullptr;
    default_im_layout_ = nullptr;
    auto_layout_ = false;
  }

  void set_timeout_dur(const Clock::duration& dur) {
    has_timeout_dur_ = true;
    timeout_dur_ = dur;
  }

  void set_default_layout(
      std::shared_ptr<const KeyboardLayout> layout) noexcept {
    default_layout_ = std::move(layout);
  }

  void set_default_im_layout(
      std::shared_ptr<const KeyboardLayout> layout) noexcept {
    default_im_layout_ = std::move(layout);
  }

  void set_auto_layout(bool is_enabled) noexcept { auto_layout_ = is_enabled; }

  std::shared_ptr<KeyboardLayout> create_layout(const std::string& name) {
    FUJINAMI_LOG(debug, "create_layout (name:{})", name);
    auto sp = std::make_shared<KeyboardLayout>(name);
    layouts_.push_back(sp);
    return sp;
  }

 private:
  bool has_timeout_dur_ = false;
  Clock::duration timeout_dur_ = Clock::duration::zero();
  std::vector<std::shared_ptr<KeyboardLayout>> layouts_;
  std::shared_ptr<const KeyboardLayout> default_layout_;
  std::shared_ptr<const KeyboardLayout> default_im_layout_;
  bool auto_layout_ = false;
};
}  // namespace fujinami
