#pragma once

#include <deque>
#include <gsl/gsl>
#include <fujinami/logging.hpp>
#include <fujinami/keyboard_config.hpp>
#include <fujinami/keyboard_layout.hpp>
#include "event.hpp"

namespace fujinami {
namespace buffering {
class State final {
 public:
  State() {}

  ~State() noexcept {}

  void press_none_key(Key key) noexcept {
    active_keyset_.reset();
    trigger_keyset_.reset();
    dontcare_keyset_ += key;
  }

  bool try_release_trigger_key(Key key) noexcept {
    if (!trigger_keyset_[key]) return false;
    active_keyset_.reset();
    trigger_keyset_.reset();
    dontcare_keyset_ -= key;
    return true;
  }

  bool try_release_modifier_key(Key key) noexcept {
    if (!modifier_keyset_[key]) return false;
    active_keyset_ -= key;
    modifier_keyset_ -= key;
    dontcare_keyset_ -= key;
    return true;
  }

  bool try_release_dontcare_key(Key key) noexcept {
    if (!dontcare_keyset_[key]) return false;
    dontcare_keyset_ -= key;
    return true;
  }

  void apply(const Keyset& active_keyset, const Keyset& trigger_keyset,
             const Keyset& modifier_keyset, Key dontcare_key) noexcept {
    active_keyset_ = active_keyset;
    trigger_keyset_ = trigger_keyset;
    modifier_keyset_ = modifier_keyset;
    dontcare_keyset_ += dontcare_key;
  }

  void apply(const Keyset& active_keyset, const Keyset& trigger_keyset,
             const Keyset& modifier_keyset,
             const Keyset& dontcare_keyset) noexcept {
    active_keyset_ = active_keyset;
    trigger_keyset_ = trigger_keyset;
    modifier_keyset_ = modifier_keyset;
    dontcare_keyset_ = dontcare_keyset;
  }

  void reset(std::shared_ptr<const KeyboardConfig> config = nullptr) noexcept {
    config_ = std::move(config);
    layout_ = config_ ? config_->default_layout() : nullptr;
    active_keyset_.reset();
    trigger_keyset_.reset();
    modifier_keyset_.reset();
    dontcare_keyset_.reset();
  }

  const KeyProperty* find_key_property(Key key) const noexcept {
    if (!layout_) return nullptr;
    return layout_->find_key_property(key);
  }

  const KeysetProperty* find_keyset_property(const Keyset& keyset) const
      noexcept {
    if (!layout_) return nullptr;
    return layout_->find_keyset_property(keyset);
  }

  void set_layout(std::shared_ptr<const KeyboardLayout> layout) noexcept {
    layout_ = std::move(layout);
  }

  void set_next_layout() noexcept {
    if (layout_) {
      auto next_layout = layout_->find_next_layout(active_keyset_).lock();
      if (next_layout) next_layout = std::move(next_layout);
    }
  }

  void push_event(const AnyEvent& event) { events_.push_back(event); }

  void pop_event() { events_.pop_front(); }

  void consume_events(size_t last) {
    events_.erase(events_.begin(), events_.begin() + last);
  }

  const std::shared_ptr<const KeyboardConfig>& config() const noexcept {
    return config_;
  }

  const std::shared_ptr<const KeyboardLayout>& layout() const noexcept {
    return layout_;
  }

  const std::deque<AnyEvent>& events() const noexcept { return events_; }

  const Keyset& active_keyset() const noexcept { return active_keyset_; }

  const Keyset& trigger_keyset() const noexcept { return trigger_keyset_; }

  const Keyset& modifier_keyset() const noexcept { return modifier_keyset_; }

  const Keyset& dontcare_keyset() const noexcept { return dontcare_keyset_; }

 private:
  std::shared_ptr<const KeyboardConfig> config_;
  std::shared_ptr<const KeyboardLayout> layout_;

  std::deque<AnyEvent> events_;

  Keyset active_keyset_;
  Keyset trigger_keyset_;
  Keyset modifier_keyset_;
  Keyset dontcare_keyset_;
};
}  // namespace buffering
}  // namespace fujinami
