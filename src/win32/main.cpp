#include <Windows.h>
#include <shellapi.h>
#include <fujinami/logging.hpp>
#include <fujinami/time.hpp>
#include <fujinami/keyboard.hpp>
#include <fujinami/keyboard_config.hpp>
#include <fujinami/config/loader.hpp>
#include <fujinami_hook/fujinami_hook.hpp>
#include "resource.h"

namespace f = fujinami;
namespace fl = fujinami::logging;
namespace fb = fujinami::buffering;
namespace fm = fujinami::mapping;
namespace fc = fujinami::config;
namespace fh = fujinami_hook;

namespace {
constexpr UINT WM_APP_NOTIFICATION = WM_APP + 1;

#ifdef DEVEL
const wchar_t* const TITLE = L"fujinami (devel)";
const wchar_t* const CLASS_NAME = L"fujinami WNDCLASS (devel)";
const wchar_t* const WINDOW_NAME = L"fujinami (devel)";
#elif defined(NDEBUG)
const wchar_t* const TITLE = L"fujinami";
const wchar_t* const CLASS_NAME = L"fujinami WNDCLASS";
const wchar_t* const WINDOW_NAME = L"fujinami";
#else
const wchar_t* const TITLE = L"fujinami (debug)";
const wchar_t* const CLASS_NAME = L"fujinami WNDCLASS (debug)";
const wchar_t* const WINDOW_NAME = L"fujinami (debug)";
#endif

FILE* stdout_fp = nullptr;
ATOM wc_atom = 0;
HWND hwnd = NULL;
HHOOK kbdll_hhook = NULL;
HMENU notification_hmenu = NULL;
HICON notification_hicon = NULL;

std::atomic<bool> do_passthrough{false};
f::Keyboard keyboard;
std::shared_ptr<f::KeyboardConfig> keyboard_config;
}  // namespace

bool init(HINSTANCE) noexcept;
void terminate(HINSTANCE) noexcept;
bool add_notification_icon(HWND) noexcept;
void delete_notification_icon() noexcept;
void show_notification_balloon(DWORD, const wchar_t*) noexcept;
void update_notification_menu(const f::KeyboardConfig&) noexcept;
LRESULT CALLBACK kbdll_hook_proc(int code, WPARAM wparam,
                                 LPARAM lparam) noexcept;
LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam,
                             LPARAM lparam) noexcept;

int APIENTRY wWinMain(HINSTANCE hinstance, HINSTANCE, LPWSTR, int) {
  if (!init(hinstance)) return EXIT_FAILURE;

  // main loop
  MSG msg{};
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  terminate(hinstance);
  return static_cast<int>(msg.wParam);
}

bool init(HINSTANCE hinstance) noexcept {
  // ロガー
  fl::Logger::init();
  fl::Logger::init_tls("H");

  // 複数の起動を防止する。
  if (FindWindow(CLASS_NAME, WINDOW_NAME) != NULL) {
    FUJINAMI_LOG(error, "application instance already exists");
    return EXIT_SUCCESS;
  }

  // リソース
  notification_hmenu =
      GetSubMenu(LoadMenu(hinstance, MAKEINTRESOURCE(IDR_CONTEXT_MENU)), 0);
  notification_hicon = LoadIcon(NULL, IDI_APPLICATION);
  // notification_hicon = LoadIcon(hinstance, IDR_NOTIFICATION_ICON);

  // ウィンドウ
  const WNDCLASS wc{
      0,
      window_proc,
      0,
      0,
      hinstance,
      notification_hicon,
      LoadCursor(NULL, IDC_ARROW),
      HBRUSH(COLOR_WINDOW + 1),
      NULL,
      CLASS_NAME,
  };
  wc_atom = RegisterClass(&wc);
  if (wc_atom == 0) return NULL;
  hwnd = CreateWindow(MAKEINTATOM(wc_atom), WINDOW_NAME, WS_OVERLAPPED,
                      CW_USEDEFAULT, CW_USEDEFAULT, 1, 1, NULL, NULL, hinstance,
                      0);

  // 通知アイコン
  if (!add_notification_icon(hwnd)) {
    FUJINAMI_LOG(error, "failed to add notification icon");
    return EXIT_FAILURE;
  }

  // KeyboardLayout
  try {
    keyboard_config = std::make_shared<f::KeyboardConfig>();
    fc::LuaLoader loader;
    loader.load(*keyboard_config);
  } catch (std::exception& e) {
    FUJINAMI_LOG(error, "failed to load: {}", e.what());
    keyboard_config.reset();
    do_passthrough = true;
  }

  // Keyboard
  try {
    keyboard.open();
    keyboard.send_event(fb::ControlEvent(keyboard_config));
  } catch (std::exception& e) {
    FUJINAMI_LOG(error, "failed to open a keyboard: {}", e.what());
    delete_notification_icon();
    return false;
  }

  // keyboard hook
  kbdll_hhook = fh::Enable(WH_KEYBOARD_LL, kbdll_hook_proc, 0);
  if (!kbdll_hhook) {
    FUJINAMI_LOG(error, "failed to enable low-level keyboard hook");
    delete_notification_icon();
    return false;
  }

  // 読み込んだレイアウトをメニューに載せる。
  if (keyboard_config) update_notification_menu(*keyboard_config);

  if (do_passthrough) {
    show_notification_balloon(NIIF_ERROR, L"設定の読み込みに失敗しました。");
  } else {
    show_notification_balloon(NIIF_INFO, L"設定の読み込みに成功しました。");
  }
  return true;
}

void terminate(HINSTANCE hinstance) noexcept {
  // hook
  fh::Disable(kbdll_hhook);
  kbdll_hhook = NULL;

  // Keyboard
  keyboard.close();

  // 通知アイコン
  delete_notification_icon();

  // ウィンドウ
  DestroyWindow(hwnd);
  UnregisterClass(MAKEINTATOM(wc_atom), hinstance);

  // ロガー
  fl::Logger::terminate();
}

bool add_notification_icon(HWND hwnd) noexcept {
  NOTIFYICONDATA nid{
      sizeof(NOTIFYICONDATA),
  };
  nid.hWnd = hwnd;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
  nid.uID = 1;
  nid.uCallbackMessage = WM_APP_NOTIFICATION;
  nid.hIcon = notification_hicon;
  nid.uVersion = NOTIFYICON_VERSION_4;
  lstrcpyn(nid.szTip, TITLE, sizeof(nid.szTip) / sizeof(*nid.szTip));

  using namespace std::chrono_literals;
  constexpr size_t RETRY_COUNT = 5;
  constexpr auto RETRY_INTERVAL = 100ms;
  bool result = false;
  for (size_t i = 0; i < RETRY_COUNT; ++i) {
    // 処理の高負荷などにより登録が上手くいかない場合の対策 (KB418138)
    if (Shell_NotifyIcon(NIM_ADD, &nid)) {
      result = true;
      break;
    }
    if (GetLastError() != ERROR_TIMEOUT) return false;
    std::this_thread::sleep_for(RETRY_INTERVAL);
    if (Shell_NotifyIcon(NIM_MODIFY, &nid)) {
      result = true;
      break;
    }
  }
  if (!result) return false;
  if (!Shell_NotifyIcon(NIM_SETVERSION, &nid)) {
    return false;
  }
  return true;
}

void delete_notification_icon() noexcept {
  NOTIFYICONDATA nid{sizeof(NOTIFYICONDATA)};
  nid.uFlags = 0;
  nid.hWnd = hwnd;
  nid.uID = 1;
  Shell_NotifyIcon(NIM_DELETE, &nid);
}

void show_notification_balloon(DWORD info_flags, const wchar_t* info) noexcept {
  NOTIFYICONDATA nid{sizeof(NOTIFYICONDATA)};
  nid.uFlags = NIF_INFO | NIF_SHOWTIP;
  nid.hWnd = hwnd;
  nid.uID = 1;
  nid.dwInfoFlags = info_flags | NIIF_RESPECT_QUIET_TIME;
  lstrcpyn(nid.szInfo, info, 256);
  lstrcpyn(nid.szInfoTitle, TITLE, 64);
  Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void hide_notification_balloon() noexcept {
  NOTIFYICONDATA nid{sizeof(NOTIFYICONDATA)};
  nid.uFlags = NIF_SHOWTIP;
  nid.hWnd = hwnd;
  nid.uID = 1;
  Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void update_notification_menu(const f::KeyboardConfig& config) noexcept {
  HMENU im_off_layout_menu = CreatePopupMenu();
  HMENU im_on_layout_menu = CreatePopupMenu();
  constexpr size_t max_layout_count = 10;
  const size_t size = std::min(max_layout_count, config.layout_count());
  for (size_t i = 0; i < size; ++i) {
    const auto layout = config.layout(i);
    if (layout) {
      const bool is_checked = layout == config.default_layout();
      AppendMenuA(im_off_layout_menu, MF_STRING | (is_checked ? MF_CHECKED : 0),
                  IDM_LAYOUT0 + i, layout->name());
      const bool is_checked_im = layout == config.default_im_layout();
      AppendMenuA(im_on_layout_menu,
                  MF_STRING | (is_checked_im ? MF_CHECKED : 0),
                  IDM_IM_LAYOUT0 + i, layout->name());
    }
  }
  ModifyMenuA(notification_hmenu, 0, MF_BYPOSITION | MF_POPUP | MF_STRING,
              (UINT_PTR)im_off_layout_menu, "IM無効時レイアウト");
  ModifyMenuA(notification_hmenu, 1, MF_BYPOSITION | MF_POPUP | MF_STRING,
              (UINT_PTR)im_on_layout_menu, "IM有効時レイアウト");
}

bool reload_keyboard_config() {
  FUJINAMI_LOG(debug, "load new config");
  std::shared_ptr<f::KeyboardConfig> new_keyboard_config;
  try {
    new_keyboard_config = std::make_shared<f::KeyboardConfig>();
    fc::LuaLoader loader;
    loader.load(*new_keyboard_config);
  } catch (std::exception& e) {
    FUJINAMI_LOG(error, "failed to reload: {}", e.what());
    return false;
  }

  keyboard.send_event(fb::ControlEvent(new_keyboard_config));
  update_notification_menu(*new_keyboard_config);
  keyboard_config = std::move(new_keyboard_config);
  return true;
}

FUJINAMI_LOGGING_DEFINE_PRINT(
    inline, KBDLLHOOKSTRUCT, data,
    (os << "{vk:" << data.vkCode;
     os << ",scan:" << std::uppercase << data.scanCode; {
       os << ",flags:0x" << std::hex << std::uppercase << data.flags << std::dec
          << ";[";
       fl::Separator sep;
       if (data.flags & LLKHF_EXTENDED) os << sep << "EXTENDED";
       if (data.flags & LLKHF_INJECTED) os << sep << "INJECTED";
       if (data.flags & LLKHF_ALTDOWN) os << sep << "ALTDOWN";
       if (data.flags & LLKHF_UP) os << sep << "UP";
       if (data.flags & LLKHF_LOWER_IL_INJECTED)
         os << sep << "LOWER_IL_INJECTED";
       os << ']';
     } os << ",time:"
          << data.time;
     os << ",extra:" << data.dwExtraInfo; os << '}';));

LRESULT CALLBACK kbdll_hook_proc(int code, WPARAM wparam,
                                 LPARAM lparam) noexcept {
  switch (code) {
    case HC_ACTION: {
      const KBDLLHOOKSTRUCT* data = reinterpret_cast<KBDLLHOOKSTRUCT*>(lparam);
      if (!data) break;
      if (data->flags & LLKHF_LOWER_IL_INJECTED ||
          data->flags & LLKHF_INJECTED) {
        FUJINAMI_LOG(trace, "injected (data:{})", *data);
        break;
      }
      // if (data->vkCode == VK_PAUSE) {
      //  PostQuitMessage(EXIT_SUCCESS);
      //  break;
      //}
      if (data->vkCode == VK_SCROLL) {
        if (!(data->flags & LLKHF_UP)) {
          if (do_passthrough) {
            FUJINAMI_LOG(trace, "passthrough disabled (data:{})", *data);
          } else {
            FUJINAMI_LOG(trace, "passthrough enabled (data:{})", *data);
          }
          do_passthrough = !do_passthrough;
        }
        break;
      }
      if (!do_passthrough) {
        FUJINAMI_LOG(trace, "send event (data:{})", *data);

        const auto time = f::Clock::time_point(f::Clock::duration(data->time));
        if (data->vkCode == VK_OEM_AUTO || data->vkCode == VK_OEM_ENLW) {
          if (!(data->flags & LLKHF_UP)) {
            const f::Key key = f::Key::HANKAKU_ZENKAKU;
            if (!keyboard.send_event(fb::KeyPressEvent(time, key)) ||
                !keyboard.send_event(fb::KeyReleaseEvent(time, key))) {
              FUJINAMI_LOG(warn, "queue is full");
            }
          }
        } else if (data->vkCode == VK_CAPITAL) {
          if (!(data->flags & LLKHF_UP)) {
            const f::Key key = f::to_key(VK_CAPITAL, false);
            if (!keyboard.send_event(fb::KeyPressEvent(time, key)) ||
                !keyboard.send_event(fb::KeyReleaseEvent(time, key))) {
              FUJINAMI_LOG(warn, "queue is full");
            }
          }
        } else if (data->vkCode == VK_KANA) {
          if (!(data->flags & LLKHF_UP)) {
            const f::Key key = f::to_key(VK_KANA, false);
            if (!keyboard.send_event(fb::KeyPressEvent(time, key)) ||
                !keyboard.send_event(fb::KeyReleaseEvent(time, key))) {
              FUJINAMI_LOG(warn, "queue is full");
            }
          }
        } else {
          const f::Key key = f::to_key(static_cast<WORD>(data->vkCode),
                                       !!(data->flags & LLKHF_EXTENDED));
          if (data->flags & LLKHF_UP) {
            if (!keyboard.send_event(fb::KeyReleaseEvent(time, key))) {
              FUJINAMI_LOG(warn, "queue is full");
            }
          } else {
            if (!keyboard.send_event(fb::KeyPressEvent(time, key))) {
              FUJINAMI_LOG(warn, "queue is full");
            }
          }
        }
        return TRUE;
      }
      break;
    }
  }
  return CallNextHookEx(kbdll_hhook, code, wparam, lparam);
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam,
                             LPARAM lparam) noexcept {
  switch (msg) {
    case WM_COMMAND: {
      const UINT id = LOWORD(wparam);
      if (id >= IDM_LAYOUT0 && id <= IDM_LAYOUT_MAX) {
        const size_t i = id - IDM_LAYOUT0;
        if (keyboard_config) {
          keyboard_config->set_default_layout(keyboard_config->layout(i));
          keyboard.send_event(
              fb::DefaultLayoutEvent(keyboard_config->default_layout(),
                                     keyboard_config->default_im_layout()));
          update_notification_menu(*keyboard_config);
        }
      } else if (id >= IDM_IM_LAYOUT0 && id <= IDM_IM_LAYOUT_MAX) {
        const size_t i = id - IDM_IM_LAYOUT0;
        if (keyboard_config) {
          keyboard_config->set_default_im_layout(keyboard_config->layout(i));
          keyboard.send_event(
              fb::DefaultLayoutEvent(keyboard_config->default_layout(),
                                     keyboard_config->default_im_layout()));
          update_notification_menu(*keyboard_config);
        }
      }
      switch (id) {
        case IDM_RELOAD: {
          if (!reload_keyboard_config()) {
            show_notification_balloon(NIIF_ERROR,
                                      L"設定の再読み込みに失敗しました。");
          } else {
            show_notification_balloon(NIIF_INFO,
                                      L"設定の再読み込みに成功しました。");
          }
          break;
        }
        case IDM_EXIT: {
          PostQuitMessage(EXIT_SUCCESS);
          break;
        }
      }
      return 0;
    }
    case WM_APP_NOTIFICATION: {
      switch (LOWORD(lparam)) {
        case WM_CONTEXTMENU: {
          SetForegroundWindow(hwnd);
          TrackPopupMenuEx(notification_hmenu, TPM_RIGHTBUTTON, LOWORD(wparam),
                           HIWORD(wparam), hwnd, NULL);
          PostMessage(hwnd, WM_NULL, 0, 0);
          break;
        }
        case NIN_BALLOONHIDE: {
          hide_notification_balloon();
          break;
        }
      }
      return 0;
    }
  }
  return DefWindowProc(hwnd, msg, wparam, lparam);
}
