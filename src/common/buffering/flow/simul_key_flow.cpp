#include <fujinami/buffering/flow/simul.hpp>
#include <fujinami/logging.hpp>
#include <fujinami/buffering/state.hpp>
#include <fujinami/buffering/event.hpp>
#include <fujinami/buffering/flow/result.hpp>

namespace fujinami {
namespace buffering {
FlowResult SimulKeyFlow::reset(State& state) noexcept {
  if (state.events().empty() ||
      state.events().front().type() != EventType::KEY_PRESS) {
    return FlowResult::DONE;
  }
  const KeyPressEvent& front_event = state.events().front().as<KeyPressEvent>();

  const Keyset active_keyset = state.modifier_keyset() + front_event.key();
  const KeysetProperty* keyset_property =
      state.find_keyset_property(active_keyset);

  FUJINAMI_LOG(trace, "begin SIMUL flow");
  const auto timeout_dur = state.config() ? (state.config()->timeout_dur()) : Clock::duration::zero();
  if (timeout_dur < Clock::duration::max()) {
    timeout_tp_ = front_event.time() + timeout_dur;
  } else {
    timeout_tp_ = Clock::time_point::max();
  }
  press_timeout_tp_ = front_event.time() + timeout_dur / 2;  // TODO:
  release_timeout_tp_ = front_event.time() + timeout_dur / 2;  // TODO:
  observed_event_last_ = 0;  // front_eventはpopするので0から始める。
  modifier_keyset_ = state.modifier_keyset();
  dontcare_keyset_ = state.dontcare_keyset() + front_event.key();
  pre_released_keyset_.reset();
  post_released_keyset_.reset();
  first_key_ = front_event.key();
  first_begin_tp_ = front_event.time();
  first_end_tp_ = Clock::time_point::max();
  second_key_ = Key::UNKNOWN;
  second_begin_tp_ = Clock::time_point::max();
  second_dontcare_keyset_.reset();
  second_post_released_keyset_.reset();
  third_key_ = Key::UNKNOWN;
  third_begin_tp_ = Clock::time_point::max();
  state.pop_event();
  return FlowResult::CONTINUE;
}

FlowResult SimulKeyFlow::update(State& state) noexcept {
  if (observed_event_last_ >= state.events().size()) {
    // 覗き見るイベントが存在しないとき
    const auto now = Clock::now();

    // 第1キーがタイムアウトした場合、
    // キーを離したとして扱い、状態を更新して処理を終了する。
    if (timeout_tp_ <= now) {
      FUJINAMI_LOG(trace, "timed out (timeout:{}, now:{})",
                   timeout_tp_, now);
      first_end_tp_ = timeout_tp_;
      consume(state);
      return FlowResult::DONE;
    }

    return FlowResult::CONTINUE;
  } else {
    // 覗き見るイベントが存在するとき
    const AnyEvent& any_event = state.events()[observed_event_last_++];
    //FUJINAMI_LOG(trace, "update (any_event:{}, index:{})",
    //             any_event, observed_event_last_ - 1);

    switch (any_event.type()) {
      case EventType::KEY_PRESS: {
        const auto& event = any_event.as<KeyPressEvent>();

        // 第1キーがタイムアウトした場合、
        // キーを離したとして扱い、状態を更新して処理を終了する。
        if (timeout_tp_ <= event.time()) {
          FUJINAMI_LOG(trace, "timed out (timeout:{}, event:{})",
                       timeout_tp_, event);
          first_end_tp_ = timeout_tp_;
          consume(state);
          return FlowResult::DONE;
        }

        // 異なるフロータイプのキーが挟まれた場合、状態を更新して処理を終了する。
        const KeyProperty* key_property = state.find_key_property(event.key());
        if (!key_property || key_property->flow_type() != FlowType::SIMUL) {
          FUJINAMI_LOG(trace, "interrupt (event:{})", event);
          first_end_tp_ = event.time();
          consume(state);
          return FlowResult::DONE;
        }

        // 第1キーのリピートが発生した場合、
        // キーを離したとして扱い、状態を更新して処理を終了する。
        if (event.key() == first_key_) {
          FUJINAMI_LOG(trace, "repeat (event:{})", event);
          first_end_tp_ = event.time();
          consume(state);
          return FlowResult::DONE;
        }

        // 無視するキーでない場合、キーを登録する。
        if (!dontcare_keyset_[event.key()]) {
          if (third_key_ != Key::UNKNOWN) {
            // do nothing
            FUJINAMI_LOG(trace, "more keys (event:{})", event);
          } else if (second_key_ != Key::UNKNOWN) {
            FUJINAMI_LOG(trace, "register third key (event:{}, dontcare:{}, post_released:{})",
                         event, dontcare_keyset_, post_released_keyset_);
            dontcare_keyset_ += event.key();
            post_released_keyset_ -= event.key();
            third_key_ = event.key();
            third_begin_tp_ = event.time();
            //third_consumed_event_last_ = observed_event_last_;
            //third_dontcare_keyset_ = dontcare_keyset_;
            //third_post_released_keyset_ = post_released_keyset_;
          } else {
            FUJINAMI_LOG(trace, "register second key (event:{}, dontcare:{}, post_released:{})",
                         event, dontcare_keyset_, post_released_keyset_);
            dontcare_keyset_ += event.key();
            post_released_keyset_ -= event.key();
            second_key_ = event.key();
            second_begin_tp_ = event.time();
            second_consumed_event_last_ = observed_event_last_;
            second_dontcare_keyset_ = dontcare_keyset_;
            second_post_released_keyset_ = post_released_keyset_;
          }
        }
        return FlowResult::CONTINUE;
      }
      case EventType::KEY_RELEASE: {
        const auto& event = any_event.as<KeyReleaseEvent>();

        // 第1キーがタイムアウトした場合、
        // キーを離したとして扱い、状態を更新して処理を終了する。
        if (timeout_tp_ <= event.time()) {
          FUJINAMI_LOG(trace, "timed out (timeout:{}, event:{})",
                       timeout_tp_, event);
          first_end_tp_ = timeout_tp_;
          consume(state);
          return FlowResult::DONE;
        }

        // 異なるフロータイプのキーが挟まれた場合、状態を更新して処理を終了する。
        const KeyProperty* key_property = state.find_key_property(event.key());
        if (!key_property || key_property->flow_type() != FlowType::SIMUL) {
          FUJINAMI_LOG(trace, "interrupt (event:{})", event);
          first_end_tp_ = event.time();
          consume(state);
          return FlowResult::DONE;
        }

        // 第1キーを離した場合、状態を更新して処理を終了する。
        if (event.key() == first_key_) {
          FUJINAMI_LOG(trace, "release (event:{})", event);
          first_end_tp_ = event.time();
          consume(state);
          return FlowResult::DONE;
        }

        if (modifier_keyset_[event.key()]) {
          // 修飾キーを離したとき
          if (event.time() < release_timeout_tp_) {
            // イベント時刻が指定範囲内である場合、事前リリースを行うためにキーを記録する。
            FUJINAMI_LOG(trace, "pre-release (event:{})", event);
            pre_released_keyset_ -= event.key();
          } else {
            // イベント時刻が指定範囲外である場合、事後リリースを行うためにキーを記録する。
            FUJINAMI_LOG(trace, "post-release (event:{})", event);
            post_released_keyset_ += event.key();
          }
          modifier_keyset_ -= event.key();
          dontcare_keyset_ -= event.key();
        } else if (dontcare_keyset_[event.key()]) {
          FUJINAMI_LOG(trace, "release dontcare (event:{})", event);
          dontcare_keyset_ -= event.key();
        }
        return FlowResult::CONTINUE;
      }
    }

    // キーイベント以外を挟んだ場合、状態を更新して処理を終了する。
    // TODO: キーイベント以外でイベント時刻を取得する？
    FUJINAMI_LOG(trace, "non-key event (event:{})", any_event);
    first_end_tp_ = Clock::now();
    consume(state);
    return FlowResult::DONE;
  }
}

bool SimulKeyFlow::is_idle(const State& state) const noexcept {
  return observed_event_last_ == state.events().size();
}

Clock::time_point SimulKeyFlow::timeout_tp() const noexcept {
  return timeout_tp_;
}

void SimulKeyFlow::consume(State& state) noexcept {
  // 第1キーと第2キーが同時打鍵しているかを調べる。
  bool is_simul = false;
  if (second_key_ != Key::UNKNOWN) {
    if (third_key_ != Key::UNKNOWN) {
      const auto p1 = second_begin_tp_ - first_begin_tp_;
      const auto p3 = third_begin_tp_ - second_begin_tp_;
      if (p1 <= p3 &&
          second_begin_tp_ < press_timeout_tp_) {
        is_simul = true;
      }
    } else {
      if (second_begin_tp_ < press_timeout_tp_) {
        is_simul = true;
      }
    }
  }

  // 同時打鍵しているが同時押しを登録していない場合、第1キーを単打したと解釈する。  
  // TODO: もっと同時打鍵に優しい方法を考える
  if (is_simul) {
    const Keyset fixed_modifier_keyset = state.modifier_keyset() - pre_released_keyset_;
    const Keyset active_keyset = fixed_modifier_keyset + first_key_ + second_key_;
    const KeysetProperty* keyset_property = state.find_keyset_property(active_keyset);
    if (!keyset_property || !keyset_property->is_mapped()) {
      is_simul = false;
    }
  }

  if (is_simul) {
    // 第1キーと第2キーが同時打鍵したとして、状態を更新する。
    const Keyset fixed_modifier_keyset = state.modifier_keyset() - pre_released_keyset_;
    const Keyset active_keyset = fixed_modifier_keyset + first_key_ + second_key_;
    const KeysetProperty* keyset_property = state.find_keyset_property(active_keyset);
    if (keyset_property && keyset_property->is_mapped()) {
      FUJINAMI_LOG(trace, "mapped (keyset:{})", active_keyset);
      state.apply(active_keyset,
                  keyset_property->trigger_keyset(),
                  keyset_property->modifier_keyset() - second_post_released_keyset_,
                  second_dontcare_keyset_);
    } else {
      FUJINAMI_LOG(trace, "unregistered or unmapped (keyset:{})", active_keyset);
      state.apply(Keyset{},
                  Keyset{},
                  fixed_modifier_keyset - second_post_released_keyset_,
                  second_dontcare_keyset_);
    }
    state.consume_events(second_consumed_event_last_);
  } else {
    // 第1キーを単打したとして、状態を更新する。
    const Keyset active_keyset = state.modifier_keyset() - pre_released_keyset_ + first_key_;
    const KeysetProperty* keyset_property = state.find_keyset_property(active_keyset);
    if (keyset_property && keyset_property->is_mapped()) {
      FUJINAMI_LOG(trace, "mapped (keyset:{})", active_keyset);
      state.apply(active_keyset,
                  keyset_property->trigger_keyset(),
                  keyset_property->modifier_keyset(),
                  first_key_);
    } else {
      FUJINAMI_LOG(trace, "unregistered or unmapped (keyset:{})", active_keyset);
      state.press_none_key(first_key_);
    }
  }
}
}  // namespace buffering
}  // namespace fujinami
