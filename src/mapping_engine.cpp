#include <fujinami/mapping/engine.hpp>
#include <fujinami/logging.hpp>

namespace fujinami {
namespace mapping {
Engine::Engine() noexcept {}

Engine::~Engine() noexcept {
  if (prev_command_) prev_command_->release();
}

void Engine::update(const AnyEvent& event) noexcept {
  switch (event.type()) {
    case EventType::KEY_PRESS:
      update(event.as<KeyPressEvent>());
      break;
    case EventType::KEY_REPEAT:
      update(event.as<KeyRepeatEvent>());
      break;
    case EventType::KEY_RELEASE:
      update(event.as<KeyReleaseEvent>());
      break;
    case EventType::LAYOUT:
      update(event.as<LayoutEvent>());
      break;
  }
}

void Engine::reset() noexcept {
  if (prev_command_) {
    prev_command_->release();
    prev_command_ = nullptr;
  }
  layout_ = nullptr;
}

void Engine::update(const KeyPressEvent& event) noexcept {
  FUJINAMI_LOG(debug, "press (event:{})", event);

  const Command* command = nullptr;
  if (layout_) {
    command = layout_->find_command(event.active_keyset());
  }

  FUJINAMI_LOG(trace, "execute command (prev?:{}, next?:{})", prev_command_,
               command);
  if (command) {
    command->press(prev_command_);
    prev_command_ = command;
  } else {
    if (prev_command_) {
      prev_command_->release();
      prev_command_ = nullptr;
    }
  }
}
void Engine::update(const KeyRepeatEvent& event) noexcept {
  FUJINAMI_LOG(debug, "repeat (event:{})", event);

  const Command* command = nullptr;
  if (layout_) {
    command = layout_->find_command(event.active_keyset());
  }

  FUJINAMI_LOG(trace, "execute command (prev?:{}, next?:{})", prev_command_,
               command);
  if (command) {
    command->repeat(prev_command_);
    prev_command_ = command;
  } else {
    if (prev_command_) {
      prev_command_->release();
      prev_command_ = nullptr;
    }
  }
}

void Engine::update(const KeyReleaseEvent& event) noexcept {
  FUJINAMI_LOG(debug, "release (event:{})", event);

  if (prev_command_) {
    FUJINAMI_LOG(trace, "execute command (prev?:{})", prev_command_);
    prev_command_->release();
    prev_command_ = nullptr;
  }
}

void Engine::update(const LayoutEvent& event) noexcept {
  FUJINAMI_LOG(debug, "reset layout (event:{})", event);
  layout_ = event.layout();
}
}  // namespace mapping
}  // namespace fujinami
