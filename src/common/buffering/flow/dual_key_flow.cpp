#include <fujinami/buffering/flow/dual.hpp>
#include <fujinami/logging.hpp>
#include <fujinami/buffering/event.hpp>
#include <fujinami/buffering/state.hpp>
#include <fujinami/buffering/flow/result.hpp>

namespace fujinami {
namespace buffering {
FlowResult DualKeyFlow::reset(State& state) noexcept {
  assert(!state.events().empty() &&
         state.events().front().type() == EventType::KEY_PRESS);
  const KeyPressEvent& front_event = state.events().front().as<KeyPressEvent>();

  FUJINAMI_LOG(trace, "begin DUAL flow");
  modifier_keyset_ = state.modifier_keyset();
  dontcare_keyset_ = state.dontcare_keyset() + front_event.key();
  first_key_ = front_event.key();
  state.pop_event();
  return FlowResult::CONTINUE;
}

FlowResult DualKeyFlow::update(State& state) noexcept {
  if (state.events().empty()) {
    return FlowResult::CONTINUE;
  } else {
    const AnyEvent& any_event = state.events().front();

    switch (any_event.type()) {
      case EventType::KEY_PRESS: {
        const auto& event = any_event.as<KeyPressEvent>();

        // 第1キーと異なるキーを押した場合、第1キーを修飾キーと見なして処理を終了する。
        if (event.key() != first_key_) {
          FUJINAMI_LOG(trace, "as modifier (event:{})", event);
          finish(state, true);
          return FlowResult::DONE;
        }

        // 第1キーと同じキーを押した場合、破棄して次のキーを待つ。
        FUJINAMI_LOG(trace, "repeat (event:{})", event);
        state.pop_event();
        return FlowResult::CONTINUE;
      }
      case EventType::KEY_RELEASE: {
        const auto& event = any_event.as<KeyReleaseEvent>();

        // 第1キーと異なるキーを離した場合、通常処理する。
        if (event.key() != first_key_) {
          FUJINAMI_LOG(trace, "release (event:{})", event);
          modifier_keyset_ -= event.key();
          dontcare_keyset_ -= event.key();
          return FlowResult::CONTINUE;
        }

        // 第1キーと同じキーを離した場合、第1キーをトリガーキーと見なして処理する。
        FUJINAMI_LOG(trace, "as trigger (event:{})", event);
        finish(state, false);
        return FlowResult::DONE;
      }
    }

    // キーイベント以外を挟んだ場合、第1キーをトリガーキーと見なして処理を終了する。
    FUJINAMI_LOG(trace, "non-key event (event:{})", any_event);
    finish(state, false);
    return FlowResult::DONE;
  }
}

void DualKeyFlow::finish(State& state, bool mod) noexcept {
  if (mod) {
    state.apply(state.modifier_keyset(),
                Keyset{},
                modifier_keyset_ + first_key_,
                dontcare_keyset_);
  } else {
    const Keyset active_keyset = state.modifier_keyset() + first_key_;
    const KeysetProperty* keyset_property = nullptr;
    if (state.layout()) {
      keyset_property = state.layout()->find_keyset_property(active_keyset);
    }
    if (keyset_property && keyset_property->is_mapped()) {
      // active_keysetにマッピングが存在する場合、
      // active_keysetに登録されている状態に更新する。
      FUJINAMI_LOG(trace, "mapped (keyset:{})", active_keyset);
      state.apply(active_keyset,
                  keyset_property->trigger_keyset(),
                  keyset_property->modifier_keyset(),
                  dontcare_keyset_);
    } else {
      // active_keysetにマッピングが存在しない場合、状態を初期化する。
      FUJINAMI_LOG(trace, "unregistered or unmapped (keyset:{})", active_keyset);
      state.apply(Keyset{},
                  Keyset{},
                  modifier_keyset_,
                  dontcare_keyset_);
    }
  }
}

bool DualKeyFlow::is_idle(const State& state) const noexcept {
  return state.events().empty();
}
}  // namespace buffering
}  // namespace fujinami
