#pragma once

#include <fujinami/logging.hpp>
#include "../state.hpp"
#include "../event.hpp"
#include "result.hpp"

namespace fujinami {
namespace buffering {
// 連続したキーイベントを同時押しとして解釈するフロー
class DeferredKeyFlow final {
 public:
  FlowResult reset(State& state) noexcept;

  FlowResult update(State& state) noexcept;

  bool is_idle(const State& state) const noexcept;

  Clock::time_point timeout_tp() const noexcept;

 private:
  FlowResult update(const KeyPressEvent& event, State& state) noexcept;
  FlowResult update(const KeyReleaseEvent& event, State& state) noexcept;

  Clock::time_point timeout_tp_;
  size_t observed_event_last_ = 0;  // 次に覗き見るイベントを指す
  size_t consumed_event_last_ = 0;  // 処理後に削除するイベントの終端を指す
  Key repeat_key_ = Key::UNKNOWN;  // キーリピートの対象となるキー
  Keyset pressed_keyset_;  // 押されている有効なキーのセット
  Keyset dontcare_keyset_;  // 処理終了時の状態を記録する無視するキーのセット
  const KeysetProperty* keyset_property_ = nullptr;
};
}  // namespace buffering
}  // namespace fujinami
