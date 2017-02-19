#pragma once

#include <array>
#include <cuchar>
#include <gsl/gsl>
#include "../../logging.hpp"
#include "../../flagset.hpp"
#include "../../key.hpp"

namespace fujinami {
namespace {
struct CINPUT final : INPUT {
  CINPUT(WORD vk, bool is_extended, DWORD other_flags = 0) noexcept {
    type = INPUT_KEYBOARD;
    ki.wVk = vk;
    ki.wScan = 0;
    ki.dwFlags = other_flags;
    if (is_extended) ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    ki.time = 0;
    ki.dwExtraInfo = 0;
  }

  CINPUT(WORD scan) noexcept {
    type = INPUT_KEYBOARD;
    ki.wVk = 0;
    ki.wScan = scan;
    ki.dwFlags = KEYEVENTF_UNICODE;
    ki.time = 0;
    ki.dwExtraInfo = 0;
  }

  void send() noexcept { SendInput(1, this, sizeof(INPUT)); }
};

std::array<Modifier, 8> MODIFIER_FLAGS{
    Modifier::SHIFT_LEFT,    Modifier::SHIFT_RIGHT, Modifier::CONTROL_LEFT,
    Modifier::CONTROL_RIGHT, Modifier::ALT_LEFT,    Modifier::ALT_RIGHT,
    Modifier::OS_LEFT,       Modifier::OS_RIGHT,
};

std::array<CINPUT, 8> MODIFIER_PRESS_INPUTS{
    CINPUT(VK_LSHIFT, false),   CINPUT(VK_RSHIFT, true),
    CINPUT(VK_LCONTROL, false), CINPUT(VK_RCONTROL, true),
    CINPUT(VK_LMENU, false),    CINPUT(VK_RMENU, true),
    CINPUT(VK_LWIN, true),      CINPUT(VK_RWIN, true),
};

std::array<CINPUT, 8> MODIFIER_RELEASE_INPUTS{
    CINPUT(VK_LSHIFT, false, KEYEVENTF_KEYUP),
    CINPUT(VK_RSHIFT, true, KEYEVENTF_KEYUP),
    CINPUT(VK_LCONTROL, false, KEYEVENTF_KEYUP),
    CINPUT(VK_RCONTROL, true, KEYEVENTF_KEYUP),
    CINPUT(VK_LMENU, false, KEYEVENTF_KEYUP),
    CINPUT(VK_RMENU, true, KEYEVENTF_KEYUP),
    CINPUT(VK_LWIN, true, KEYEVENTF_KEYUP),
    CINPUT(VK_RWIN, true, KEYEVENTF_KEYUP),
};
}  // namespace

class KeyAction {
 public:
  explicit KeyAction(Key key, Modifiers modifiers) noexcept
      : modifiers_(modifiers) {
    std::tie(vk_, is_extended_) = to_keycode(key);
  }

  void press(const KeyAction& prev) const noexcept {
    prev.release_key();
    update_modifiers(prev);
    press_key();
  }

  void press() const noexcept {
    press_modifiers();
    press_key();
  }

  void repeat(const KeyAction& prev) const noexcept {
    prev.release_key(vk_, is_extended_);
    update_modifiers(prev);
    repeat_key();
  }

  void repeat() const noexcept {
    press_modifiers();
    repeat_key();
  }

  void release() const noexcept {
    release_key();
    release_modifiers();
  }

  FUJINAMI_LOGGING_STRUCT(
      KeyAction,
      (("vk", vk_))(("is_extended", is_extended_))(("modifiers", modifiers_)));

 private:
  void press_key() const noexcept {
    if (vk_ != 0) {
      const bool is_numlocked = !!(GetKeyState(VK_NUMLOCK) & 0x0001);
      CINPUT(apply_numlock(vk_, is_numlocked), is_extended_).send();
    }
  }

  void repeat_key() const noexcept {
    if (vk_ != 0) {
      const bool is_numlocked = !!(GetKeyState(VK_NUMLOCK) & 0x0001);
      CINPUT(apply_numlock(vk_, is_numlocked), is_extended_).send();
    }
  }

  void release_key(WORD next_vk, bool next_is_extended) const noexcept {
    if (vk_ != 0 && (vk_ != next_vk || is_extended_ != next_is_extended)) {
      CINPUT(vk_, is_extended_, KEYEVENTF_KEYUP).send();
    }
  }

  void release_key() const noexcept {
    if (vk_ != 0) {
      CINPUT(vk_, is_extended_, KEYEVENTF_KEYUP).send();
    }
  }

  void update_modifiers(const KeyAction& prev) const noexcept {
    const Modifiers keyup_modifiers = prev.modifiers_ & ~modifiers_;
    const Modifiers keydown_modifiers = ~prev.modifiers_ & modifiers_;
    for (size_t i = 0; i < MODIFIER_FLAGS.size(); ++i) {
      if (keyup_modifiers & MODIFIER_FLAGS[i]) {
        MODIFIER_RELEASE_INPUTS[i].send();
      } else if (keydown_modifiers & MODIFIER_FLAGS[i]) {
        MODIFIER_PRESS_INPUTS[i].send();
      }
    }
  }

  void press_modifiers() const noexcept {
    for (size_t i = 0; i < MODIFIER_FLAGS.size(); ++i) {
      if (modifiers_ & MODIFIER_FLAGS[i]) {
        MODIFIER_PRESS_INPUTS[i].send();
      }
    }
  }

  void release_modifiers() const noexcept {
    for (size_t i = 0; i < MODIFIER_FLAGS.size(); ++i) {
      if (modifiers_ & MODIFIER_FLAGS[i]) {
        MODIFIER_RELEASE_INPUTS[i].send();
      }
    }
  }

  WORD vk_ = 0;
  bool is_extended_ = false;
  Modifiers modifiers_{};
};

class CharAction final {
 public:
  explicit CharAction(char16_t c) noexcept : char_(c) {}

  void press(const CharAction&) const noexcept { press(); }

  void press() const noexcept {
    if (char_ != u'\0') {
      CINPUT input(static_cast<WORD>(char_));
      SendInput(1, &input, sizeof(INPUT));
    }
  }

  void repeat(const CharAction&) const noexcept { press(); }

  void repeat() const noexcept { press(); }

  void release() const noexcept {}

  FUJINAMI_LOGGING_STRUCT(CharAction, (("char", char_)));

 private:
  char16_t char_ = u'\0';
};
}  // namespace fujinami
