#pragma once

#include <cstdint>
#include "../logging.hpp"
#include "../keyset.hpp"

namespace fujinami {
namespace mapping {
enum class EventType : uint8_t {
  NONE,
  KEY_PRESS,
  KEY_REPEAT,
  KEY_RELEASE,
  LAYOUT,
};
FUJINAMI_LOGGING_ENUM(inline, EventType,
                      (NONE)(KEY_PRESS)(KEY_REPEAT)(KEY_RELEASE)(LAYOUT));

class KeyPressEvent {
 public:
  KeyPressEvent() = default;

  explicit KeyPressEvent(const Keyset& active_keyset) noexcept
      : active_keyset_(active_keyset) {}

  const Keyset& active_keyset() const noexcept { return active_keyset_; }

  FUJINAMI_LOGGING_STRUCT(KeyPressEvent, (("active_keyset", active_keyset_)));

 private:
  Keyset active_keyset_;
};

class KeyRepeatEvent {
 public:
  KeyRepeatEvent() = default;

  explicit KeyRepeatEvent(const Keyset& active_keyset) noexcept
      : active_keyset_(active_keyset) {}

  const Keyset& active_keyset() const noexcept { return active_keyset_; }

  FUJINAMI_LOGGING_STRUCT(KeyRepeatEvent, (("active_keyset", active_keyset_)));

 private:
  Keyset active_keyset_;
};

class KeyReleaseEvent {
 public:
  KeyReleaseEvent() = default;

  explicit KeyReleaseEvent(const Keyset& active_keyset) noexcept
      : active_keyset_(active_keyset) {}

  const Keyset& active_keyset() const noexcept { return active_keyset_; }

  FUJINAMI_LOGGING_STRUCT(KeyReleaseEvent, (("active_keyset", active_keyset_)));

 private:
  Keyset active_keyset_;
};

class LayoutEvent final {
 public:
  LayoutEvent() = default;

  explicit LayoutEvent(std::shared_ptr<const KeyboardLayout> layout) noexcept
      : layout_(layout) {}

  const std::shared_ptr<const KeyboardLayout>& layout() const noexcept {
    return layout_;
  }

  FUJINAMI_LOGGING_STRUCT(LayoutEvent, (("layout", layout_)));

 private:
  std::shared_ptr<const KeyboardLayout> layout_;
};

class AnyEvent final {
 public:
  static constexpr bool is_nothrow_copy_constructible =
      std::is_nothrow_copy_constructible<KeyPressEvent>::value &&
      std::is_nothrow_copy_constructible<KeyRepeatEvent>::value &&
      std::is_nothrow_copy_constructible<KeyReleaseEvent>::value &&
      std::is_nothrow_copy_constructible<LayoutEvent>::value;
  static constexpr bool is_nothrow_copy_assignable =
      is_nothrow_copy_constructible &&
      std::is_nothrow_copy_assignable<KeyPressEvent>::value &&
      std::is_nothrow_copy_assignable<KeyRepeatEvent>::value &&
      std::is_nothrow_copy_assignable<KeyReleaseEvent>::value &&
      std::is_nothrow_copy_assignable<LayoutEvent>::value;

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

  AnyEvent(const KeyRepeatEvent& other) noexcept(
      std::is_nothrow_copy_constructible<KeyRepeatEvent>::value) {
    new (&key_repeat_) KeyRepeatEvent(other);
    type_ = EventType::KEY_REPEAT;
  }

  AnyEvent(KeyRepeatEvent&& other) noexcept {
    new (&key_repeat_) KeyRepeatEvent(std::move(other));
    type_ = EventType::KEY_REPEAT;
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

  AnyEvent(const LayoutEvent& other) noexcept(
      std::is_nothrow_copy_constructible<LayoutEvent>::value) {
    new (&layout_) LayoutEvent(other);
    type_ = EventType::LAYOUT;
  }
  AnyEvent(LayoutEvent&& other) noexcept(
      std::is_nothrow_copy_constructible<LayoutEvent>::value) {
    new (&layout_) LayoutEvent(std::move(other));
    type_ = EventType::LAYOUT;
  }

  ~AnyEvent() noexcept { destruct(); }

  AnyEvent& operator=(const AnyEvent& other) noexcept(
      is_nothrow_copy_assignable) {
    if (type_ == other.type_) {
      switch (type_) {
        case EventType::KEY_PRESS:
          key_press_ = other.key_press_;
          break;
        case EventType::KEY_REPEAT:
          key_repeat_ = other.key_repeat_;
          break;
        case EventType::KEY_RELEASE:
          key_release_ = other.key_release_;
          break;
        case EventType::LAYOUT:
          layout_ = other.layout_;
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
        case EventType::KEY_REPEAT:
          key_repeat_ = std::move(other.key_repeat_);
          break;
        case EventType::KEY_RELEASE:
          key_release_ = std::move(other.key_release_);
          break;
        case EventType::LAYOUT:
          layout_ = std::move(other.layout_);
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
                             (KEY_REPEAT, "key_repeat", key_repeat_))(
                             (KEY_RELEASE, "key_release",
                              key_release_))((LAYOUT, "layout", layout_)));

 private:
  void construct(const AnyEvent& other) noexcept(
      is_nothrow_copy_constructible) {
    switch (other.type_) {
      case EventType::KEY_PRESS:
        new (&key_press_) KeyPressEvent(other.key_press_);
        break;
      case EventType::KEY_REPEAT:
        new (&key_repeat_) KeyRepeatEvent(other.key_repeat_);
        break;
      case EventType::KEY_RELEASE:
        new (&key_release_) KeyReleaseEvent(other.key_release_);
        break;
      case EventType::LAYOUT:
        new (&layout_) LayoutEvent(other.layout_);
        break;
    }
    type_ = other.type_;
  }

  void construct(AnyEvent&& other) noexcept {
    switch (other.type_) {
      case EventType::KEY_PRESS:
        new (&key_press_) KeyPressEvent(std::move(other.key_press_));
        break;
      case EventType::KEY_REPEAT:
        new (&key_repeat_) KeyRepeatEvent(std::move(other.key_repeat_));
        break;
      case EventType::KEY_RELEASE:
        new (&key_release_) KeyReleaseEvent(std::move(other.key_release_));
        break;
      case EventType::LAYOUT:
        new (&layout_) LayoutEvent(std::move(other.layout_));
        break;
    }
    type_ = other.type_;
  }

  void destruct() noexcept {
    switch (type_) {
      case EventType::KEY_PRESS:
        key_press_.~KeyPressEvent();
        break;
      case EventType::KEY_REPEAT:
        key_repeat_.~KeyRepeatEvent();
        break;
      case EventType::KEY_RELEASE:
        key_release_.~KeyReleaseEvent();
        break;
      case EventType::LAYOUT:
        layout_.~LayoutEvent();
        break;
    }
  }

  EventType type_ = EventType::NONE;
  union {
    KeyPressEvent key_press_;
    KeyRepeatEvent key_repeat_;
    KeyReleaseEvent key_release_;
    LayoutEvent layout_;
  };
};
template <>
inline const KeyPressEvent& AnyEvent::as() const noexcept {
  return key_press_;
}

template <>
inline const KeyRepeatEvent& AnyEvent::as() const noexcept {
  return key_repeat_;
}

template <>
inline const KeyReleaseEvent& AnyEvent::as() const noexcept {
  return key_release_;
}

template <>
inline const LayoutEvent& AnyEvent::as() const noexcept {
  return layout_;
}
}  // namespace mapping
}  // namespace fujinami
