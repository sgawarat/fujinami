#pragma once

#include <gsl/gsl>
#include <unistd.h>
#include <linux/input.h>

namespace fujinami {
class Input final {
 public:
  static bool init(gsl::czstring event_path,
                   gsl::czstring uinput_path) noexcept;
  static void terminate() noexcept;
  static bool receive(input_event& event) noexcept;
  static void send(__u16 code, __s32 value) noexcept;

  static void send_press(__u16 code) noexcept { send(code, 1); }

  static void send_repeat(__u16 code) noexcept { send(code, 2); }

  static void send_release(__u16 code) noexcept { send(code, 0); }

  static void send_input(const input_event& ie) noexcept {
    input_event sent_ie = ie;
    gettimeofday(&sent_ie.time, nullptr);
    write(uifd_, &sent_ie, sizeof(sent_ie));
  }

 private:
  static int epfd_;
  static int evfd_;
  static int uifd_;
};
}  // namespace fujinami
