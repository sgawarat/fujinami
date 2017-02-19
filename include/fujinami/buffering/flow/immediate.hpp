#pragma once

#include "../../logging.hpp"
#include "../event.hpp"
#include "../state.hpp"
#include "result.hpp"

namespace fujinami {
namespace buffering {
// 単独押しするキーを処理するフロー
class ImmediateKeyFlow final {
 public:
  FlowResult reset(State& state) noexcept;

  FlowResult update(State&) noexcept {
    assert(!"UNREACHABLE");
    return FlowResult::DONE;
  }

  bool is_idle(const State&) const noexcept { return true; }

 private:
};
}  // namespace buffering
}  // namespace fujinami
