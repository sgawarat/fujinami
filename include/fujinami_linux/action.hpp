#pragma once

#include <array>
#include <gsl/gsl>
#include "../../logging.hpp"
#include "../../flagset.hpp"
#include "../../key.hpp"
#include "input.hpp"

namespace fujinami {
namespace {
constexpr std::array<Modifier, 4> MODIFIER_LEFT_FLAGS{
    Modifier::SHIFT_LEFT, Modifier::CONTROL_LEFT, Modifier::ALT_LEFT,
    Modifier::OS_LEFT,
};
constexpr std::array<Modifier, 4> MODIFIER_RIGHT_FLAGS{
    Modifier::SHIFT_RIGHT, Modifier::CONTROL_RIGHT, Modifier::ALT_RIGHT,
    Modifier::OS_RIGHT,
};
constexpr std::array<uint16_t, 4> MODIFIER_LEFT_KEYCODES{
    KEY_LEFTSHIFT, KEY_LEFTCTRL, KEY_LEFTALT, KEY_LEFTMETA,
};
constexpr std::array<uint16_t, 4> MODIFIER_RIGHT_KEYCODES{
    KEY_RIGHTSHIFT, KEY_RIGHTCTRL, KEY_RIGHTALT, KEY_RIGHTMETA,
};
}  // namespace

class KeyAction {
 public:
  explicit KeyAction(Key key, Modifiers modifiers) noexcept
      : code_(to_keycode(key)), modifiers_(modifiers) {}

  void press(const KeyAction& prev) const noexcept {
    // リピート中でないならば、必ずリリースイベントを発生させる。
    if (prev.code_ != 0) Input::send_release(prev.code_);
    update_modifiers(prev);
    if (code_ != 0) Input::send_press(code_);
  }

  void press() const noexcept {
    update_modifiers();
    if (code_ != 0) Input::send_press(code_);
  }

  void repeat(const KeyAction& prev) const noexcept {
    // リピート中ならば、直前と異なるキーのみリリースイベントを発生させる。
    if (prev.code_ != 0 && code_ != prev.code_) Input::send_release(prev.code_);
    update_modifiers(prev);
    if (code_ != 0) {
      if (code_ != prev.code_) {
        Input::send_press(code_);
      } else {
        Input::send_repeat(code_);
      }
    }
  }

  void repeat() const noexcept {
    update_modifiers();
    if (code_ != 0) Input::send_repeat(code_);
  }

  void release() const noexcept { cleanup(); }

  FUJINAMI_LOGGING_STRUCT(KeyAction,
                          (("code", code_))(("modifiers", modifiers_)));

 private:
  void update_modifiers(const KeyAction& prev) const noexcept {
    const Modifiers keyup_modifiers = prev.modifiers_ & ~modifiers_;
    const Modifiers keydown_modifiers = ~prev.modifiers_ & modifiers_;
    for (size_t i = 0; i < MODIFIER_LEFT_FLAGS.size(); ++i) {
      if (keyup_modifiers & MODIFIER_LEFT_FLAGS[i]) {
        Input::send_release(MODIFIER_LEFT_KEYCODES[i]);
      } else if (keydown_modifiers & MODIFIER_LEFT_FLAGS[i]) {
        Input::send_press(MODIFIER_LEFT_KEYCODES[i]);
      }
    }
    for (size_t i = 0; i < MODIFIER_RIGHT_FLAGS.size(); ++i) {
      if (keyup_modifiers & MODIFIER_RIGHT_FLAGS[i]) {
        Input::send_release(MODIFIER_RIGHT_KEYCODES[i]);
      } else if (keydown_modifiers & MODIFIER_RIGHT_FLAGS[i]) {
        Input::send_press(MODIFIER_RIGHT_KEYCODES[i]);
      }
    }
  }

  void update_modifiers() const noexcept {
    for (size_t i = 0; i < MODIFIER_LEFT_FLAGS.size(); ++i) {
      if (modifiers_ & MODIFIER_LEFT_FLAGS[i]) {
        Input::send_press(MODIFIER_LEFT_KEYCODES[i]);
      }
    }
    for (size_t i = 0; i < MODIFIER_RIGHT_FLAGS.size(); ++i) {
      if (modifiers_ & MODIFIER_RIGHT_FLAGS[i]) {
        Input::send_press(MODIFIER_RIGHT_KEYCODES[i]);
      }
    }
  }

  void cleanup() const noexcept {
    for (size_t i = 0; i < MODIFIER_LEFT_FLAGS.size(); ++i) {
      if (modifiers_ & MODIFIER_LEFT_FLAGS[i]) {
        Input::send_release(MODIFIER_LEFT_KEYCODES[i]);
      }
    }
    for (size_t i = 0; i < MODIFIER_RIGHT_FLAGS.size(); ++i) {
      if (modifiers_ & MODIFIER_RIGHT_FLAGS[i]) {
        Input::send_release(MODIFIER_RIGHT_KEYCODES[i]);
      }
    }
    if (code_ != 0) Input::send_release(code_);
  }

  __u16 code_ = 0;
  Modifiers modifiers_;
};

class CharAction final {
 public:
  explicit CharAction(char16_t c) noexcept : char_(c) {}

  void press(const CharAction&) const noexcept { press(); }

  void press() const noexcept { assert(!"NOIMPL"); }

  void repeat(const CharAction&) const noexcept { press(); }

  void repeat() const noexcept { press(); }

  void release() const noexcept { assert(!"NOIMPL"); }

  FUJINAMI_LOGGING_STRUCT(CharAction, (("char", char_)));

 private:
  char16_t char_ = u'\0';
};
}  // namespace fujinami
