#pragma once

#include <fujinami/flagset.hpp>
#include <fujinami/logging.hpp>
#include "../state.hpp"
#include "../event.hpp"
#include "result.hpp"

namespace fujinami {
namespace buffering {
// 同時打鍵を実現するフロー
class SimulKeyFlow final {
 public:
  FlowResult reset(State& state) noexcept;

  FlowResult update(State& state) noexcept;

  bool is_idle(const State& state) const noexcept;

  Clock::time_point timeout_tp() const noexcept;

 private:
  void consume(State& state) noexcept;

  Clock::time_point timeout_tp_;
  Clock::time_point press_timeout_tp_;
  Clock::time_point release_timeout_tp_;
  size_t observed_event_last_;
  Keyset modifier_keyset_;
  Keyset dontcare_keyset_;
  Keyset pre_released_keyset_;
  Keyset post_released_keyset_;

  // はじめに押されるキー
  Key first_key_;
  Clock::time_point first_begin_tp_;
  Clock::time_point first_end_tp_;

  // 次に押されるキー
  Key second_key_;
  Clock::time_point second_begin_tp_;
  size_t second_consumed_event_last_;
  Keyset second_dontcare_keyset_;
  Keyset second_post_released_keyset_;

  // 最後に押されるキー
  Key third_key_;
  Clock::time_point third_begin_tp_;
};
}  // namespace buffering
}  // namespace fujinami
