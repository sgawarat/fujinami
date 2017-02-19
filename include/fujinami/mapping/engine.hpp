#pragma once

#include <gsl/gsl>
#include "../command.hpp"
#include "../keyboard_config.hpp"
#include "../keyboard_layout.hpp"
#include "event.hpp"

namespace fujinami {
namespace mapping {
class Engine final {
 public:
  Engine(const Engine&) = delete;
  Engine(Engine&&) = delete;
  Engine& operator=(const Engine&) = delete;
  Engine& operator=(Engine&&) = delete;

  Engine() noexcept;

  ~Engine() noexcept;

  void update(const AnyEvent& event) noexcept;

  void reset() noexcept;

 private:
  void update(const KeyPressEvent& event) noexcept;
  void update(const KeyRepeatEvent& event) noexcept;
  void update(const KeyReleaseEvent& event) noexcept;
  void update(const LayoutEvent& event) noexcept;

  std::shared_ptr<const KeyboardLayout> layout_;
  const Command* prev_command_ = nullptr;
};
}  // namespace mapping
}  // namespace fujinami
