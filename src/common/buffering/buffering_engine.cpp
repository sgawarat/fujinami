#include <fujinami/buffering/engine.hpp>
#include <fujinami/logging.hpp>
#include <fujinami/time.hpp>
#include <fujinami/mapping/context.hpp>

namespace fujinami {
namespace buffering {
namespace {
#ifdef FUJINAMI_PLATFORM_WIN32
inline bool get_im_status() noexcept {
  GUITHREADINFO gui_thread_info{sizeof(GUITHREADINFO)};
  GetGUIThreadInfo(0, &gui_thread_info);
  const HWND hwnd = ImmGetDefaultIMEWnd(gui_thread_info.hwndFocus);
  return SendMessage(hwnd, WM_IME_CONTROL, 5 /*IMC_GETOPENSTATUS*/, 0) != 0;
}
#endif
}  // namespace

Engine::Engine() {}

Engine::~Engine() noexcept {}

void Engine::update(NextStageContext& context) noexcept {
  switch (current_flow_) {
    case FlowType::UNKNOWN: {
      if (state_.events().empty()) break;
      const AnyEvent& event = state_.events().front();
      switch (event.type()) {
        case EventType::KEY_PRESS: {
          update(event.as<KeyPressEvent>(), context);
          break;
        }
        case EventType::KEY_RELEASE: {
          update(event.as<KeyReleaseEvent>(), context);
          break;
        }
        case EventType::DEFAULT_LAYOUT: {
          update(event.as<DefaultLayoutEvent>(), context);
          break;
        }
        case EventType::CONTROL: {
          update(event.as<ControlEvent>(), context);
          break;
        }
      }
      break;
    }
    case FlowType::IMMEDIATE: {
      FUJINAMI_LOGGING_SECTION("IMMEDIATE");
      if (immediate_key_flow_.update(state_) == FlowResult::CONTINUE) break;
      state_.set_next_layout();
      context.send_press(state_.active_keyset(), state_.layout());
      current_flow_ = FlowType::UNKNOWN;
      break;
    }
    case FlowType::DEFERRED: {
      FUJINAMI_LOGGING_SECTION("DEFERRED");
      if (deferred_key_flow_.update(state_) == FlowResult::CONTINUE) break;
      state_.set_next_layout();
      context.send_press(state_.active_keyset(), state_.layout());
      current_flow_ = FlowType::UNKNOWN;
      break;
    }
    case FlowType::SIMUL: {
      FUJINAMI_LOGGING_SECTION("SIMUL");
      if (simul_key_flow_.update(state_) == FlowResult::CONTINUE) break;
      state_.set_next_layout();
      context.send_press(state_.active_keyset(), state_.layout());
      current_flow_ = FlowType::UNKNOWN;
      break;
    }
    case FlowType::DUAL: {
      FUJINAMI_LOGGING_SECTION("DUAL");
      if (dual_key_flow_.update(state_) == FlowResult::CONTINUE) break;
      state_.set_next_layout();
      context.send_press(state_.active_keyset(), state_.layout());
      current_flow_ = FlowType::UNKNOWN;
      break;
    }
  }
}

void Engine::update(const AnyEvent& event, NextStageContext& context) noexcept {
  state_.push_event(event);
  update(context);
}

bool Engine::is_idle() const noexcept {
  switch (current_flow_) {
    case FlowType::IMMEDIATE:
      return immediate_key_flow_.is_idle(state_);
    case FlowType::DEFERRED:
      return deferred_key_flow_.is_idle(state_);
    case FlowType::SIMUL:
      return simul_key_flow_.is_idle(state_);
    case FlowType::DUAL:
      return dual_key_flow_.is_idle(state_);
  }
  return state_.events().empty();
}

void Engine::reset() noexcept {
  default_layout_ = nullptr;
  default_im_layout_ = nullptr;
  auto_layout_ = false;
  prev_im_status_ = false;
  state_.reset();
  current_flow_ = FlowType::UNKNOWN;
}

Clock::time_point Engine::timeout_tp() const noexcept {
  switch (current_flow_) {
    case FlowType::IMMEDIATE:
      return immediate_key_flow_.timeout_tp();
    case FlowType::DEFERRED:
      return deferred_key_flow_.timeout_tp();
    case FlowType::SIMUL:
      return simul_key_flow_.timeout_tp();
    case FlowType::DUAL:
      return dual_key_flow_.timeout_tp();
  }
  return Clock::time_point::max();
}

void Engine::update(const KeyPressEvent& event,
                    NextStageContext& context) noexcept {
  FUJINAMI_LOG(debug, "press (event:{})", event);

  if (state_.trigger_keyset() && state_.active_keyset()[event.key()]) {
    FUJINAMI_LOG(trace, "repeat (active_keyset:{})", state_.active_keyset());
    context.send_repeat(state_.active_keyset());
    state_.pop_event();
    return;
  }

  if (state_.dontcare_keyset()[event.key()]) {
    FUJINAMI_LOG(trace, "ignore (active_keyset:{})", state_.active_keyset());
    state_.pop_event();
    return;
  }

#ifdef FUJINAMI_PLATFORM_WIN32
  // IMの状態の変化に応じてレイアウトを切り替える。
  if (auto_layout_) {
    const bool im_status = get_im_status();
    // IMの状態が変化していた場合、現在の状態に対応するレイアウトをセットする。
    if (prev_im_status_ && !im_status) {
      FUJINAMI_LOG(trace, "IM is disabled");
      state_.set_layout(default_layout_);
      context.send_layout(default_layout_);
    } else if (!prev_im_status_ && im_status) {
      FUJINAMI_LOG(trace, "IM is enabled");
      state_.set_layout(default_im_layout_);
      context.send_layout(default_im_layout_);
    }
    prev_im_status_ = im_status;
  }
#endif

  // 登録されたフローにキーイベントを投げる。
  const KeyProperty* key_property = state_.find_key_property(event.key());
  if (!key_property || key_property->flow_type() == FlowType::UNKNOWN) {
    FUJINAMI_LOG(trace, "press unregistered key");
    state_.press_none_key(event.key());
    state_.pop_event();
    return;
  }
  switch (key_property->flow_type()) {
    case FlowType::IMMEDIATE: {
      FUJINAMI_LOG(trace, "reset IMMEDIATE flow");
      FUJINAMI_LOGGING_SECTION("IMMEDIATE");
      switch (immediate_key_flow_.reset(state_)) {
        case FlowResult::CONTINUE:
          current_flow_ = FlowType::IMMEDIATE;
          break;
        case FlowResult::DONE:
          state_.set_next_layout();
          context.send_press(state_.active_keyset(), state_.layout());
          break;
      }
      break;
    }
    case FlowType::DEFERRED: {
      FUJINAMI_LOG(trace, "reset DEFERRED flow");
      FUJINAMI_LOGGING_SECTION("DEFERRED");
      switch (deferred_key_flow_.reset(state_)) {
        case FlowResult::CONTINUE:
          current_flow_ = FlowType::DEFERRED;
          break;
        case FlowResult::DONE:
          state_.set_next_layout();
          context.send_press(state_.active_keyset(), state_.layout());
          break;
      }
      break;
    }
    case FlowType::SIMUL: {
      FUJINAMI_LOG(trace, "reset SIMUL flow");
      FUJINAMI_LOGGING_SECTION("SIMUL");
      switch (simul_key_flow_.reset(state_)) {
        case FlowResult::CONTINUE:
          current_flow_ = FlowType::SIMUL;
          break;
        case FlowResult::DONE:
          state_.set_next_layout();
          context.send_press(state_.active_keyset(), state_.layout());
          break;
      }
      break;
    }
    case FlowType::DUAL: {
      FUJINAMI_LOG(trace, "reset DUAL flow");
      FUJINAMI_LOGGING_SECTION("DUAL");
      switch (dual_key_flow_.reset(state_)) {
        case FlowResult::CONTINUE:
          current_flow_ = FlowType::SIMUL;
          break;
        case FlowResult::DONE:
          state_.set_next_layout();
          context.send_press(state_.active_keyset(), state_.layout());
          break;
      }
      break;
    }
  }
}

void Engine::update(const KeyReleaseEvent& event,
                    NextStageContext& context) noexcept {
  FUJINAMI_LOG(debug, "release (event:{})", event);

  if (state_.try_release_trigger_key(event.key())) {
    FUJINAMI_LOG(trace, "release trigger key");
    context.send_release(state_.active_keyset());
  } else if (state_.try_release_modifier_key(event.key())) {
    FUJINAMI_LOG(trace, "release modifier key");
    // キーリピート中でない場合のみ、リリースイベントを送る。
    if (!state_.trigger_keyset()) {
      context.send_release(state_.active_keyset());
    }
  } else if (state_.try_release_dontcare_key(event.key())) {
    FUJINAMI_LOG(trace, "release dontcare key");
  } else {
    FUJINAMI_LOG(trace, "release other key");
  }
  state_.pop_event();
}

void Engine::update(const DefaultLayoutEvent& event,
                    NextStageContext& context) noexcept {
  FUJINAMI_LOG(trace, "default_layout (event:{})", event);
  default_layout_ = event.default_layout();
  default_im_layout_ = event.default_im_layout();
  prev_im_status_ = false;
  state_.set_layout(default_layout_);
  context.send_layout(default_layout_);
  state_.pop_event();
}

void Engine::update(const ControlEvent& event,
                    NextStageContext& context) noexcept {
  FUJINAMI_LOG(trace, "control (event:{})", event);
  if (event.config()) {
    default_layout_ = event.config()->default_layout();
    default_im_layout_ = event.config()->default_im_layout();
    auto_layout_ = event.config()->auto_layout();
    prev_im_status_ = false;
    state_.reset(event.config());
    context.send_layout(default_layout_);
  } else {
    default_layout_ = nullptr;
    default_im_layout_ = nullptr;
    auto_layout_ = false;
    prev_im_status_ = false;
    state_.reset();
    context.send_layout(nullptr);
  }
  state_.pop_event();
}
}  // namespace buffering
}  // namespace fujinami
