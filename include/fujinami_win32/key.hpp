#pragma once

#include <Windows.h>
#include <array>
#include <tuple>
#include <gsl/gsl>
#include <fujinami/logging.hpp>

namespace fujinami {
enum class Key : uint8_t {
  UNKNOWN = 0,
  // 間借り
  NUMPAD_RETURN = VK_OEM_AUTO,
  HANKAKU_ZENKAKU = VK_OEM_ENLW,
};
static constexpr size_t KEY_COUNT = 256;
FUJINAMI_LOGGING_DEFINE_PRINT(inline, Key, key,
                              (os << static_cast<uint32_t>(key);));

inline Key to_key(WORD vk, bool is_extended = false) noexcept {
  // テンキーはNumLockのON/OFFに関わらず押したキーを返す
  if (!is_extended) {
    switch (vk) {
      case VK_INSERT:
        return static_cast<Key>(VK_NUMPAD0);
      case VK_END:
        return static_cast<Key>(VK_NUMPAD1);
      case VK_DOWN:
        return static_cast<Key>(VK_NUMPAD2);
      case VK_NEXT:
        return static_cast<Key>(VK_NUMPAD3);
      case VK_LEFT:
        return static_cast<Key>(VK_NUMPAD4);
      case VK_CLEAR:
        return static_cast<Key>(VK_NUMPAD5);
      case VK_RIGHT:
        return static_cast<Key>(VK_NUMPAD6);
      case VK_HOME:
        return static_cast<Key>(VK_NUMPAD7);
      case VK_UP:
        return static_cast<Key>(VK_NUMPAD8);
      case VK_PRIOR:
        return static_cast<Key>(VK_NUMPAD9);
      case VK_DELETE:
        return static_cast<Key>(VK_DECIMAL);
    }
  }

  // キーコードを共有する拡張キーは使用しないキーにマッピングする
  if (is_extended) {
    switch (vk) {
      case VK_RETURN:
        return Key::NUMPAD_RETURN;
    }
  }

  // 全角半角キーはVK_OEM_ENLWにまとめるので、VK_OEM_AUTOは全角半角キーとして使わない
  if (vk == VK_OEM_AUTO) return Key::UNKNOWN;

  // その他
  return static_cast<Key>(vk);
}

inline std::tuple<WORD, bool> to_keycode(Key key) noexcept {
  // 間借りしていた拡張キー
  switch (key) {
    case Key::NUMPAD_RETURN:
      return {VK_RETURN, true};
  }

  // 独立したキーコードを持つ拡張キー
  switch (static_cast<uint8_t>(key)) {
    case VK_INSERT:
    case VK_END:
    case VK_DOWN:
    case VK_NEXT:
    case VK_LEFT:
    case VK_CLEAR:
    case VK_RIGHT:
    case VK_HOME:
    case VK_UP:
    case VK_PRIOR:
    case VK_DELETE:
    case VK_RCONTROL:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
    case VK_SNAPSHOT:
    case VK_NUMLOCK:
    case VK_DIVIDE:
      return {static_cast<WORD>(key), true};
  }

  // その他
  return {static_cast<WORD>(key), false};
}

// NumLockの状態に応じたキーを返す
inline WORD apply_numlock(WORD vk, bool is_numlocked) {
  if (!is_numlocked) {
    switch (vk) {
      case VK_NUMPAD0:
        return VK_INSERT;
      case VK_NUMPAD1:
        return VK_END;
      case VK_NUMPAD2:
        return VK_DOWN;
      case VK_NUMPAD3:
        return VK_NEXT;
      case VK_NUMPAD4:
        return VK_LEFT;
      case VK_NUMPAD5:
        return VK_CLEAR;
      case VK_NUMPAD6:
        return VK_RIGHT;
      case VK_NUMPAD7:
        return VK_HOME;
      case VK_NUMPAD8:
        return VK_UP;
      case VK_NUMPAD9:
        return VK_PRIOR;
      case VK_DECIMAL:
        return VK_DELETE;
    }
  }
  return vk;
}
}  // namespace fujinami
