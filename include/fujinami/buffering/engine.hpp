#pragma once

#include "state.hpp"
#include "event.hpp"
#include "flow/immediate.hpp"
#include "flow/deferred.hpp"

namespace fujinami {
namespace mapping {
class Context;
}  // namespace mapping

namespace buffering {
namespace next_stage = mapping;
using NextStageContext = next_stage::Context;

class Engine {
 public:
  Engine(const Engine&) = delete;
  Engine(Engine&&) = delete;
  Engine& operator=(const Engine&) = delete;
  Engine& operator=(Engine&&) = delete;

  Engine();

  ~Engine() noexcept;

  void update(NextStageContext& context) noexcept;

  void update(const AnyEvent& event, NextStageContext& context) noexcept;

  bool is_idle() const noexcept;

  void reset() noexcept;

  bool has_timeout_tp() const noexcept { return state_.has_timeout_tp(); }

  const Clock::time_point& timeout_tp() const noexcept {
    return state_.timeout_tp();
  }

 private:
  void update(const KeyPressEvent& event, NextStageContext& context) noexcept;
  void update(const KeyReleaseEvent& event, NextStageContext& context) noexcept;
  void update(const DefaultLayoutEvent& event,
              NextStageContext& context) noexcept;
  void update(const ControlEvent& event, NextStageContext& context) noexcept;

  std::shared_ptr<const KeyboardConfig> config_;
  std::shared_ptr<const KeyboardLayout> default_layout_;
  std::shared_ptr<const KeyboardLayout> default_im_layout_;

  bool auto_layout_ = false;
  bool prev_im_status_ = false;

  State state_;
  FlowType current_flow_ = FlowType::UNKNOWN;
  ImmediateKeyFlow immediate_key_flow_;
  DeferredKeyFlow deferred_key_flow_;
};
}  // namespace buffering
}  // namespace fujinami
