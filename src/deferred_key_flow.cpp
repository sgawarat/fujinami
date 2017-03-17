#include <fujinami/buffering/flow/deferred.hpp>
#include <fujinami/logging.hpp>
#include <fujinami/buffering/state.hpp>
#include <fujinami/buffering/event.hpp>
#include <fujinami/buffering/flow/result.hpp>

namespace fujinami {
namespace buffering {
FlowResult DeferredKeyFlow::reset(State& state) noexcept {
  assert(!state.events().empty() &&
         state.events().front().type() == EventType::KEY_PRESS);
  const KeyPressEvent& front_event = state.events().front().as<KeyPressEvent>();

  const Keyset active_keyset = state.modifier_keyset() + front_event.key();
  const KeysetProperty* keyset_property =
      state.find_keyset_property(active_keyset);

  // active_keysetを経由するキーセットが登録されていない場合、
  // 状態を初期化して処理を終了する。
  if (!keyset_property) {
    FUJINAMI_LOG(trace, "unregistered (keyset:{})", active_keyset);
    state.press_none_key(front_event.key());
    state.pop_event();
    return FlowResult::DONE;
  }

  if (keyset_property->is_mapped()) {
    // active_keysetにマッピングが存在する場合、
    // active_keysetに登録されている状態に更新する。
    FUJINAMI_LOG(trace, "mapped (keyset:{})", active_keyset);
    state.apply(active_keyset, keyset_property->trigger_keyset(),
                keyset_property->modifier_keyset(), front_event.key());
  } else {
    // active_keysetにマッピングが存在しない場合、状態を初期化する。
    FUJINAMI_LOG(trace, "unmapped (keyset:{})", active_keyset);
    state.press_none_key(front_event.key());
  }

  // active_keysetと組み合わせ可能なキーがこれ以上存在しない場合、
  // 現在の状態で確定して処理を終了する。
  if (keyset_property->is_leaf()) {
    FUJINAMI_LOG(trace, "leaf (keyset:{})", active_keyset);
    state.pop_event();
    return FlowResult::DONE;
  }

  // active_keysetと組み合わせ可能なキーが存在する場合、
  // 以降のイベントを含めて状態を確定してゆく。
  FUJINAMI_LOG(trace, "begin DEFERRED flow");
  const auto timeout_dur = state.config() ? (state.config()->timeout_dur()) : Clock::duration::zero();
  if (timeout_dur < Clock::duration::max()) {
    timeout_tp_ = front_event.time() + timeout_dur;
  } else {
    timeout_tp_ = Clock::time_point::max();
  }
  observed_event_last_ =
      1;  // 0番目(front_event)はすでに見たので、1から始める。
  consumed_event_last_ =
      1;  // 0番目(front_event)は必ず消費するので、1から始める。
  repeat_key_ = front_event.key();
  pressed_keyset_ = active_keyset;
  dontcare_keyset_ = state.dontcare_keyset();
  keyset_property_ = keyset_property;
  return FlowResult::CONTINUE;
}

FlowResult DeferredKeyFlow::update(State& state) noexcept {
  // 覗き見るイベントが存在しないとき
  if (observed_event_last_ == state.events().size()) {
    // 現在時刻が指定時刻を過ぎている場合、現在の状態で確定する。
    const auto now = Clock::now();
    if (timeout_tp_ <= now) {
      FUJINAMI_LOG(trace, "timed out (now:{}, timeout:{})", now,
                   timeout_tp_);
      state.consume_events(consumed_event_last_);
      return FlowResult::DONE;
    }
    return FlowResult::CONTINUE;
  }

  const AnyEvent& event = state.events()[observed_event_last_++];

  switch (event.type()) {
    case EventType::KEY_PRESS:
      return update(event.as<KeyPressEvent>(), state);
    case EventType::KEY_RELEASE:
      return update(event.as<KeyReleaseEvent>(), state);
  }

  // キーイベント以外を挟んだ場合、直ちに処理を終了する。
  state.consume_events(consumed_event_last_);
  return FlowResult::DONE;
}

bool DeferredKeyFlow::is_idle(const State& state) const noexcept {
  return observed_event_last_ == state.events().size();
}

Clock::time_point DeferredKeyFlow::timeout_tp() const noexcept {
  return timeout_tp_;
}


FlowResult DeferredKeyFlow::update(const KeyPressEvent& event,
                                   State& state) noexcept {
  FUJINAMI_LOG(debug, "press (event:{})", event);

  // イベントが指定時刻以降に届いた場合、現在の状態で確定する。
  if (timeout_tp_ <= event.time()) {
    FUJINAMI_LOG(trace, "timed out (event:{})", event);
    state.consume_events(consumed_event_last_);
    return FlowResult::DONE;
  }

  const KeyProperty* key_property = state.find_key_property(event.key());

  // 異なる属性を持つキーを押す場合、処理を終了する。
  if (!key_property || key_property->flow_type() != FlowType::DEFERRED) {
    FUJINAMI_LOG(trace, "interrupt (key_property?:{})", key_property);
    state.consume_events(consumed_event_last_);
    return FlowResult::DONE;
  }

  // システムのキーリピートが発生した場合、処理を終了する。
  if (event.key() == repeat_key_) {
    FUJINAMI_LOG(trace, "repeat");
    state.consume_events(consumed_event_last_);
    return FlowResult::DONE;
  }

  // キーが組み合わせ可能でない場合、処理を終了する。
  if (!keyset_property_->is_combinable(event.key())) {
    FUJINAMI_LOG(trace, "not-combinable (combinable_keyset:{})",
                 keyset_property_->combinable_keyset());
    state.consume_events(consumed_event_last_);
    return FlowResult::DONE;
  }

  // 組み合わせ可能なキーを押す場合、同時に押されたと解釈して状態を更新する。
  repeat_key_ = event.key();
  pressed_keyset_ += event.key();
  dontcare_keyset_ += event.key();

  keyset_property_ = state.find_keyset_property(pressed_keyset_);
  if (!keyset_property_) {
    FUJINAMI_LOG(trace, "unregistered (keyset:{})", pressed_keyset_);
    state.consume_events(consumed_event_last_);
    return FlowResult::DONE;
  }

  if (keyset_property_->is_mapped()) {
    FUJINAMI_LOG(trace, "mapped (keyset:{})", pressed_keyset_);
    state.apply(pressed_keyset_, keyset_property_->trigger_keyset(),
                keyset_property_->modifier_keyset(), dontcare_keyset_);
    consumed_event_last_ = observed_event_last_;
  }

  if (keyset_property_->is_leaf()) {
    FUJINAMI_LOG(trace, "leaf");
    state.consume_events(consumed_event_last_);
    return FlowResult::DONE;
  }

  return FlowResult::CONTINUE;
}

FlowResult DeferredKeyFlow::update(const KeyReleaseEvent& event,
                                   State& state) noexcept {
  FUJINAMI_LOG(debug, "release (event:{})", event);

  // イベントが指定時刻以降に届いた場合、現在の状態で確定する。
  if (timeout_tp_ <= event.time()) {
    FUJINAMI_LOG(trace, "timed out (event:{})", event);
    state.consume_events(consumed_event_last_);
    return FlowResult::DONE;
  }

  // 押しているキーを離す場合、現在のキーの組み合わせで確定する合図と解釈する。
  if (pressed_keyset_[event.key()]) {
    state.consume_events(consumed_event_last_);
    return FlowResult::DONE;
  }

  // 処理終了時に無視を解除するキーを記録する。
  dontcare_keyset_ -= event.key();

  return FlowResult::CONTINUE;
}
}  // namespace buffering
}  // namespace fujinami
