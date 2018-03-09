#pragma once

#include <fujinami/logging.hpp>
#include "../event.hpp"
#include "../state.hpp"
#include "result.hpp"

namespace fujinami {
namespace buffering {
// 単独押しを特別扱いするキーを処理するフロー
class DualKeyFlow final {
 public:
  FlowResult reset(State& state) noexcept;

  FlowResult update(State&) noexcept;

  bool is_idle(const State&) const noexcept;

  Clock::time_point timeout_tp() const noexcept {
    return Clock::time_point::max();
  }

 private:
  void finish(State& state, bool mod) noexcept;

  Keyset modifier_keyset_;
  Keyset dontcare_keyset_;
  Key first_key_;
};
}  // namespace buffering
}  // namespace fujinami
