#pragma once

#include <new>
#include "../logging.hpp"
#include "../key.hpp"
#include "../time.hpp"
#include "../keyboard_config.hpp"
#include "../keyboard_layout.hpp"

namespace fujinami {
namespace buffering {
enum class EventType : uint8_t {
  NONE,
  KEY_PRESS,
  KEY_RELEASE,
  DEFAULT_LAYOUT,
  CONTROL,
};
FUJINAMI_LOGGING_ENUM(inline, EventType,
                      (NONE)(KEY_PRESS)(KEY_RELEASE)(DEFAULT_LAYOUT)(CONTROL));

class KeyPressEvent {
 public:
  explicit KeyPressEvent(const Clock::time_point& time, Key key)
      : time_(time), key_(key) {}

  const Clock::time_point& time() const noexcept { return time_; }

  Key key() const noexcept { return key_; }

  FUJINAMI_LOGGING_STRUCT(KeyPressEvent, (("time", time_))(("key", key_)));

 private:
  Clock::time_point time_;
  Key key_ = Key::UNKNOWN;
};

class KeyReleaseEvent {
 public:
  explicit KeyReleaseEvent(const Clock::time_point& time, Key key)
      : time_(time), key_(key) {}

  const Clock::time_point& time() const noexcept { return time_; }

  Key key() const noexcept { return key_; }

  FUJINAMI_LOGGING_STRUCT(KeyReleaseEvent, (("time", time_))(("key", key_)));

 private:
  Clock::time_point time_;
  Key key_ = Key::UNKNOWN;
};

class DefaultLayoutEvent final {
 public:
  explicit DefaultLayoutEvent(
      std::shared_ptr<const KeyboardLayout> default_layout,
      std::shared_ptr<const KeyboardLayout> default_im_layout) noexcept
      : default_layout_(std::move(default_layout)),
        default_im_layout_(std::move(default_im_layout)) {}

  const std::shared_ptr<const KeyboardLayout>& default_layout() const noexcept {
    return default_layout_;
  }

  const std::shared_ptr<const KeyboardLayout>& default_im_layout() const
      noexcept {
    return default_im_layout_;
  }

  FUJINAMI_LOGGING_STRUCT(DefaultLayoutEvent,
                          (("default_layout", default_layout_))(
                              ("default_im_layout", default_im_layout_)));

 private:
  std::shared_ptr<const KeyboardLayout> default_layout_;
  std::shared_ptr<const KeyboardLayout> default_im_layout_;
};

class ControlEvent final {
 public:
  explicit ControlEvent(std::shared_ptr<const KeyboardConfig> config) noexcept
      : config_(std::move(config)) {}

  const std::shared_ptr<const KeyboardConfig>& config() const noexcept {
    return config_;
  }

  FUJINAMI_LOGGING_STRUCT(ControlEvent, (("config", config_)));

 private:
  std::shared_ptr<const KeyboardConfig> config_;
};

class AnyEvent final {
 public:
  static constexpr bool is_nothrow_copy_constructible =
      std::is_nothrow_copy_constructible<KeyPressEvent>::value &&
      std::is_nothrow_copy_constructible<KeyReleaseEvent>::value &&
      std::is_nothrow_copy_constructible<DefaultLayoutEvent>::value &&
      std::is_nothrow_copy_constructible<ControlEvent>::value;
  static constexpr bool is_nothrow_copy_assignable =
      is_nothrow_copy_constructible &&
      std::is_nothrow_copy_assignable<KeyPressEvent>::value &&
      std::is_nothrow_copy_assignable<KeyReleaseEvent>::value &&
      std::is_nothrow_copy_assignable<DefaultLayoutEvent>::value &&
      std::is_nothrow_copy_assignable<ControlEvent>::value;

  AnyEvent() noexcept : type_(EventType::NONE) {}

  AnyEvent(const AnyEvent& other) noexcept(is_nothrow_copy_constructible) {
    construct(other);
  }

  AnyEvent(AnyEvent&& other) noexcept { construct(std::move(other)); }

  AnyEvent(const KeyPressEvent& other) noexcept(
      std::is_nothrow_copy_constructible<KeyPressEvent>::value) {
    new (&key_press_) KeyPressEvent(other);
    type_ = EventType::KEY_PRESS;
  }

  AnyEvent(KeyPressEvent&& other) noexcept {
    new (&key_press_) KeyPressEvent(std::move(other));
    type_ = EventType::KEY_PRESS;
  }

  AnyEvent(const KeyReleaseEvent& other) noexcept(
      std::is_nothrow_copy_constructible<KeyReleaseEvent>::value) {
    new (&key_release_) KeyReleaseEvent(other);
    type_ = EventType::KEY_RELEASE;
  }

  AnyEvent(KeyReleaseEvent&& other) noexcept {
    new (&key_release_) KeyReleaseEvent(std::move(other));
    type_ = EventType::KEY_RELEASE;
  }

  AnyEvent(const DefaultLayoutEvent& other) noexcept(
      std::is_nothrow_copy_constructible<DefaultLayoutEvent>::value) {
    new (&default_layout_) DefaultLayoutEvent(other);
    type_ = EventType::DEFAULT_LAYOUT;
  }

  AnyEvent(DefaultLayoutEvent&& other) noexcept {
    new (&default_layout_) DefaultLayoutEvent(std::move(other));
    type_ = EventType::DEFAULT_LAYOUT;
  }

  AnyEvent(const ControlEvent& other) noexcept(
      std::is_nothrow_copy_constructible<ControlEvent>::value) {
    new (&control_) ControlEvent(other);
    type_ = EventType::CONTROL;
  }

  AnyEvent(ControlEvent&& other) noexcept {
    new (&control_) ControlEvent(std::move(other));
    type_ = EventType::CONTROL;
  }

  ~AnyEvent() noexcept { destruct(); }

  AnyEvent& operator=(const AnyEvent& other) noexcept(
      is_nothrow_copy_assignable) {
    if (type_ == other.type_) {
      switch (type_) {
        case EventType::KEY_PRESS:
          key_press_ = other.key_press_;
          break;
        case EventType::KEY_RELEASE:
          key_release_ = other.key_release_;
          break;
        case EventType::DEFAULT_LAYOUT:
          default_layout_ = other.default_layout_;
          break;
        case EventType::CONTROL:
          control_ = other.control_;
          break;
      }
    } else {
      destruct();
      type_ = EventType::NONE;
      construct(other);
    }
    return *this;
  }

  AnyEvent& operator=(AnyEvent&& other) noexcept {
    if (type_ == other.type_) {
      switch (type_) {
        case EventType::KEY_PRESS:
          key_press_ = std::move(other.key_press_);
          break;
        case EventType::KEY_RELEASE:
          key_release_ = std::move(other.key_release_);
          break;
        case EventType::DEFAULT_LAYOUT:
          default_layout_ = std::move(other.default_layout_);
          break;
        case EventType::CONTROL:
          control_ = std::move(other.control_);
          break;
      }
    } else {
      destruct();
      type_ = EventType::NONE;
      construct(std::move(other));
    }
    return *this;
  }

  EventType type() const noexcept { return type_; }

  template <typename T>
  const T& as() const noexcept;

  FUJINAMI_LOGGING_UNION(AnyEvent, EventType,
                         ((KEY_PRESS, "key_press", key_press_))(
                             (KEY_RELEASE, "key_release", key_release_))((
                             DEFAULT_LAYOUT, "default_layout",
                             default_layout_))((CONTROL, "control", control_)));

 private:
  void construct(const AnyEvent& other) noexcept(
      is_nothrow_copy_constructible) {
    switch (other.type_) {
      case EventType::KEY_PRESS:
        new (&key_press_) KeyPressEvent(other.key_press_);
        break;
      case EventType::KEY_RELEASE:
        new (&key_release_) KeyReleaseEvent(other.key_release_);
        break;
      case EventType::DEFAULT_LAYOUT:
        new (&default_layout_) DefaultLayoutEvent(other.default_layout_);
        break;
      case EventType::CONTROL:
        new (&control_) ControlEvent(other.control_);
        break;
    }
    type_ = other.type_;
  }

  void construct(AnyEvent&& other) noexcept {
    switch (other.type_) {
      case EventType::KEY_PRESS:
        new (&key_press_) KeyPressEvent(std::move(other.key_press_));
        break;
      case EventType::KEY_RELEASE:
        new (&key_release_) KeyReleaseEvent(std::move(other.key_release_));
        break;
      case EventType::DEFAULT_LAYOUT:
        new (&default_layout_)
            DefaultLayoutEvent(std::move(other.default_layout_));
        break;
      case EventType::CONTROL:
        new (&control_) ControlEvent(std::move(other.control_));
        break;
    }
    type_ = other.type_;
  }

  void destruct() noexcept {
    switch (type_) {
      case EventType::KEY_PRESS:
        key_press_.~KeyPressEvent();
        break;
      case EventType::KEY_RELEASE:
        key_release_.~KeyReleaseEvent();
        break;
      case EventType::DEFAULT_LAYOUT:
        default_layout_.~DefaultLayoutEvent();
        break;
      case EventType::CONTROL:
        control_.~ControlEvent();
        break;
    }
  }

  EventType type_ = EventType::NONE;
  union {
    KeyPressEvent key_press_;
    KeyReleaseEvent key_release_;
    DefaultLayoutEvent default_layout_;
    ControlEvent control_;
  };
};

template <>
inline const KeyPressEvent& AnyEvent::as() const noexcept {
  return key_press_;
}

template <>
inline const KeyReleaseEvent& AnyEvent::as() const noexcept {
  return key_release_;
}

template <>
inline const DefaultLayoutEvent& AnyEvent::as() const noexcept {
  return default_layout_;
}

template <>
inline const ControlEvent& AnyEvent::as() const noexcept {
  return control_;
}

}  // namespace buffering
}  // namespace fujinami
