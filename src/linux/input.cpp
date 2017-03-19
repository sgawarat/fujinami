#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <linux/uinput.h>
#include <fujinami/logging.hpp>
#include <fujinami_linux/input.hpp>

namespace fujinami {
namespace {
class FileDescriptor final {
 public:
  FileDescriptor(int fd) noexcept : fd_(fd) {}

  FileDescriptor(const FileDescriptor& other) = delete;

  FileDescriptor(FileDescriptor&& other) = delete;

  ~FileDescriptor() noexcept {
    if (fd_ >= 0) close(fd_);
  }

  FileDescriptor& operator=(const FileDescriptor& other) = delete;

  FileDescriptor& operator=(FileDescriptor&& other) = delete;

  explicit operator bool() const noexcept { return fd_ >= 0; }

  int detach() noexcept {
    if (fd_ < 0) return -1;
    const int tmp = fd_;
    fd_ = -1;
    return tmp;
  }

  int fd() const noexcept { return fd_; }

 private:
  int fd_ = -1;
};
}  // namespace

int Input::epfd_ = -1;
int Input::evfd_ = -1;
int Input::uifd_ = -1;

bool Input::init(gsl::czstring event_path, gsl::czstring uinput_path) noexcept {
  terminate();

  FileDescriptor epfd(epoll_create(1));
  if (!epfd) return false;

  FileDescriptor evfd(open(event_path, O_RDONLY | O_NONBLOCK));
  if (!evfd) return false;

  epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = evfd.fd();
  if (epoll_ctl(epfd.fd(), EPOLL_CTL_ADD, evfd.fd(), &event) < 0) return false;

  FileDescriptor uifd(open(uinput_path, O_WRONLY | O_NONBLOCK));
  if (!uifd) return false;

  uinput_user_dev user_dev{};
  strncpy(user_dev.name, "fujinami uinput", UINPUT_MAX_NAME_SIZE);
  user_dev.id.bustype = BUS_I8042;
  user_dev.id.vendor = 0xCCCC;
  user_dev.id.product = 0xCCCC;
  user_dev.id.version = 1;
  if (::write(uifd.fd(), &user_dev, sizeof(user_dev)) != sizeof(user_dev))
    return false;

  if (ioctl(uifd.fd(), UI_SET_EVBIT, EV_KEY) < 0) return false;

  // all keys without KEY_RESERVED
  for (int i = 1; i < KEY_MAX; ++i) {
    if (ioctl(uifd.fd(), UI_SET_KEYBIT, i) < 0) return false;
  }
  if (ioctl(uifd.fd(), UI_DEV_CREATE, 0) < 0) return false;

  if (ioctl(evfd.fd(), EVIOCGRAB, 1) < 0) return false;

  epfd_ = epfd.detach();
  evfd_ = evfd.detach();
  uifd_ = uifd.detach();
  return true;
}

void Input::terminate() noexcept {
  ioctl(evfd_, EVIOCGRAB, 0);

  if (epfd_ >= 0) {
    close(epfd_);
    epfd_ = -1;
  }
  if (evfd_ >= 0) {
    close(evfd_);
    evfd_ = -1;
  }
  if (uifd_ >= 0) {
    close(uifd_);
    uifd_ = -1;
  }
}

bool Input::receive(input_event& event) noexcept {
  epoll_event ee;
  if (epoll_wait(epfd_, &ee, 1, -1) != 1) return false;

  input_event ie;
  if (read(evfd_, &ie, sizeof(ie)) != sizeof(ie)) return false;

  event = ie;
  return true;
}

void Input::send(__u16 code, __s32 value) noexcept {
  input_event ie{};
  gettimeofday(&ie.time, nullptr);
  ie.type = EV_MSC;
  ie.code = MSC_SCAN;
  ie.value = code;
  write(uifd_, &ie, sizeof(ie));
  ie.type = EV_KEY;
  ie.code = code;
  ie.value = value;
  write(uifd_, &ie, sizeof(ie));
  ie.type = EV_SYN;
  ie.code = SYN_REPORT;
  ie.value = 0;
  write(uifd_, &ie, sizeof(ie));
}
}  // namespace fujinami
