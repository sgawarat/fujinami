#include <iostream>
#include <signal.h>
#include <fujinami/logging.hpp>
#include <fujinami/time.hpp>
#include <fujinami/keyboard.hpp>
#include <fujinami/keyboard_config.hpp>
#include <fujinami/platform/linux/input.hpp>
#include <fujinami/config/loader.hpp>

namespace f = fujinami;
namespace fl = fujinami::logging;
namespace fb = fujinami::buffering;
namespace fm = fujinami::mapping;
namespace fc = fujinami::config;

namespace {
std::atomic<bool> quit{false};
std::atomic<bool> do_passthrough{false};
f::Keyboard keyboard;
std::shared_ptr<f::KeyboardConfig> keyboard_config;
}  // namespace

bool init() noexcept;
void terminate() noexcept;

FUJINAMI_LOGGING_DEFINE_PRINT(inline, input_event, ie,
                              (fl::Separator sep;
                               os << sep << "time:" << ie.time;
                               os << sep << "type:" << ie.type;
                               os << sep << "code:" << ie.code;
                               os << sep << "value:" << ie.value;));

int main(int, char**) {
  // HACK: wait to flush RETURN key event
  sleep(1);

  // set SIGINT handler
  if (signal(SIGINT, [](int) { quit = true; }) == SIG_ERR) {
    perror("failed to set SIGINT handler");
    return EXIT_FAILURE;
  }

  if (!init()) return EXIT_FAILURE;

  // main loop
  input_event ie;
  while (!quit) {
    if (f::Input::receive(ie)) {
      if (do_passthrough) {
        if (ie.type == EV_KEY) {
          switch (ie.code) {
            case KEY_SCROLLLOCK:
              if (ie.value == 0) {
                if (do_passthrough) {
                  FUJINAMI_LOG(trace, "passthrough disabled (data:{})", ie);
                } else {
                  FUJINAMI_LOG(trace, "passthrough enabled (data:{})", ie);
                }
                do_passthrough = !do_passthrough;
              }
              break;
            default:
              f::Input::send_input(ie);
              break;
          }
        } else {
          f::Input::send_input(ie);
        }
      } else {
        if (ie.type == EV_KEY) {
          switch (ie.code) {
            case KEY_SCROLLLOCK:
              if (ie.value == 0) {
                if (do_passthrough) {
                  FUJINAMI_LOG(trace, "passthrough disabled (data:{})", ie);
                } else {
                  FUJINAMI_LOG(trace, "passthrough enabled (data:{})", ie);
                }
                do_passthrough = !do_passthrough;
              }
              break;
            default:
              FUJINAMI_LOG(trace, "send event (data:{})", ie);

              const auto time = f::Clock::time_point(f::to_duration(ie.time));
              const f::Key key = f::to_key(ie.code);
              if (ie.value == 0) {
                if (!keyboard.send_event(fb::KeyReleaseEvent(time, key))) {
                  FUJINAMI_LOG(warn, "queue is full");
                }
              } else {
                if (!keyboard.send_event(fb::KeyPressEvent(time, key))) {
                  FUJINAMI_LOG(warn, "queue is full");
                }
              }
              break;
          }
        }
      }
    }
    std::this_thread::yield();
  }

  terminate();
  return EXIT_SUCCESS;
}

bool init() noexcept {
  // ロガー
  fl::Logger::init();
  fl::Logger::init_tls("H");

  // 複数の起動を防止する。
  // if (...) {
  //  FUJINAMI_LOG(error, "application instance already exists");
  //  return EXIT_SUCCESS;
  //}

  // KeyboardLayout
  try {
    keyboard_config = std::make_shared<f::KeyboardConfig>();
    fc::LuaLoader loader;
    loader.load(*keyboard_config);
  } catch (std::exception& e) {
    FUJINAMI_LOG(error, "failed to load config: {}", e.what());
    return false;
  }

  // Keyboard
  try {
    keyboard.open();
    keyboard.send_event(fb::ControlEvent(keyboard_config));
  } catch (std::exception& e) {
    FUJINAMI_LOG(error, "failed to open a keyboard: {}", e.what());
    return false;
  }

  // keyboard hook
  if (!f::Input::init("/dev/input/event0", "/dev/uinput")) {
    perror("hook");
    FUJINAMI_LOG(error, "failed to enable keyboard hook");
    return false;
  }

  return true;
}

void terminate() noexcept {
  // hook
  f::Input::terminate();

  // Keyboard
  keyboard.close();
  keyboard_config = nullptr;

  // ロガー
  fl::Logger::terminate();
}
