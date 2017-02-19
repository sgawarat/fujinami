#include <fujinami/buffering/flow/immediate.hpp>
#include <fujinami/logging.hpp>
#include <fujinami/buffering/event.hpp>
#include <fujinami/buffering/state.hpp>
#include <fujinami/buffering/flow/result.hpp>

namespace fujinami {
namespace buffering {
FlowResult ImmediateKeyFlow::reset(State& state) noexcept {
  assert(!state.events().empty() &&
         state.events().front().type() == EventType::KEY_PRESS);
  const KeyPressEvent& front_event = state.events().front().as<KeyPressEvent>();

  const Keyset active_keyset = state.modifier_keyset() + front_event.key();
  const KeysetProperty* keyset_property = nullptr;
  if (state.layout()) {
    keyset_property = state.layout()->find_keyset_property(active_keyset);
  }

  if (keyset_property && keyset_property->is_mapped()) {
    // active_keysetにマッピングが存在する場合、
    // active_keysetに登録されている状態に更新する。
    FUJINAMI_LOG(trace, "mapped (keyset:{})", active_keyset);
    state.apply(active_keyset, keyset_property->trigger_keyset(),
                keyset_property->modifier_keyset(), front_event.key());
  } else {
    // active_keysetにマッピングが存在しない場合、状態を初期化する。
    FUJINAMI_LOG(trace, "unregistered or unmapped (keyset:{})", active_keyset);
    state.press_none_key(front_event.key());
  }

  // 単独で押すキーのみを取り扱うので、イベントを1つ消費したら処理を終了する。
  state.pop_event();
  return FlowResult::DONE;
}
}  // namespace buffering
}  // namespace fujinami
