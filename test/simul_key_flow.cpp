#include <catch.hpp>
#include <fujinami/buffering/flow/simul.hpp>

using namespace std::chrono_literals;
using namespace fujinami;
using namespace fujinami::buffering;

#define REQUIRE_STATE_1(t, m, dc)\
  do {\
    const size_t size = state.events().size();\
    REQUIRE(flow.reset(state) == FlowResult::CONTINUE);\
    REQUIRE(flow.update(state) == FlowResult::DONE);\
    REQUIRE(size == state.events().size() + 1);\
    REQUIRE(state.trigger_keyset() == t);\
    REQUIRE(state.modifier_keyset() == m);\
    REQUIRE(state.dontcare_keyset() == dc);\
  } while(false)

#define REQUIRE_STATE_2(c, t, m, dc)\
  do {\
    const size_t size = state.events().size();\
    REQUIRE(flow.reset(state) == FlowResult::CONTINUE);\
    REQUIRE(flow.update(state) == FlowResult::CONTINUE);\
    REQUIRE(flow.update(state) == FlowResult::DONE);\
    REQUIRE(size == state.events().size() + c);\
    REQUIRE(state.trigger_keyset() == t);\
    REQUIRE(state.modifier_keyset() == m);\
    REQUIRE(state.dontcare_keyset() == dc);\
  } while(false)

#define REQUIRE_STATE_3(c, t, m, dc)\
  do {\
    const size_t size = state.events().size();\
    REQUIRE(flow.reset(state) == FlowResult::CONTINUE);\
    REQUIRE(flow.update(state) == FlowResult::CONTINUE);\
    REQUIRE(flow.update(state) == FlowResult::CONTINUE);\
    REQUIRE(flow.update(state) == FlowResult::DONE);\
    REQUIRE(size == state.events().size() + c);\
    REQUIRE(state.trigger_keyset() == t);\
    REQUIRE(state.modifier_keyset() == m);\
    REQUIRE(state.dontcare_keyset() == dc);\
  } while(false)

TEST_CASE("SimulKeyFlow", "[fujinami][buffering][flow]") {
  const Key trigger_key_1 = to_key(1);
  const Key trigger_key_2 = to_key(2);
  const Key trigger_key_3 = to_key(3);
  const Key trigger_key_4 = to_key(4);
  const Key unmapped_key = to_key(42);
  const KeyRole trigger_key_role = KeyRole::TRIGGER;
  const KeyRole modifier_key_role = KeyRole::MODIFIER;
  const Keyset none_keyset{};
  const Keyset trigger_keyset_1{trigger_key_1};
  const Keyset trigger_keyset_12{trigger_key_1, trigger_key_2};
  const auto timeout_dur = 1000ms;
  const auto press_timeout_dur = 500ms;
  const auto release_timeout_dur = 500ms;

  auto config = std::make_shared<KeyboardConfig>();
  config->set_timeout_dur(timeout_dur);

  auto layout = config->create_layout("layout");
  layout->create_flow(trigger_key_1, FlowType::SIMUL);
  layout->create_flow(trigger_key_2, FlowType::SIMUL);
  layout->create_flow(trigger_key_3, FlowType::SIMUL);
  layout->create_mapping({trigger_key_1}, {trigger_key_role}, Command{});
  layout->create_mapping({trigger_key_1, trigger_key_2},
                         {trigger_key_role, trigger_key_role},
                         Command{});

  config->set_default_layout(layout);

  SimulKeyFlow flow;
  State state;
  const auto begin_tp = Clock::now() - timeout_dur;
  const auto middle_tp = begin_tp + press_timeout_dur;
  const auto end_tp = middle_tp + press_timeout_dur;


  SECTION("1KEY: timed out without next key") {
    // キーイベントが来ないままタイムアウトする
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    REQUIRE_STATE_1(trigger_keyset_1, none_keyset, trigger_keyset_1);
  }
  SECTION("1KEY: timed out with next key") {
    // タイムアウト後にキーイベントが来る
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{end_tp, trigger_key_2});
    REQUIRE_STATE_1(trigger_keyset_1, none_keyset, trigger_keyset_1);
  }
  SECTION("1KEY: repeat K1") {
    // キーリピートが発生する
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{begin_tp + 1ms, trigger_key_1});
    REQUIRE_STATE_1(trigger_keyset_1, none_keyset, trigger_keyset_1);
  }
  SECTION("1KEY: release K1") {
    // 第1キーを離す
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyReleaseEvent{begin_tp + 1ms, trigger_key_1});
    REQUIRE_STATE_1(trigger_keyset_1, none_keyset, trigger_keyset_1);
  }

  SECTION("2KEYS-1: timed out without next key") {
    // キーイベントが来ないままタイムアウトする
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{begin_tp + press_timeout_dur, trigger_key_2});
    REQUIRE_STATE_2(1, trigger_keyset_1, none_keyset, trigger_keyset_1);
  }
  SECTION("2KEYS-1: timed out with next key") {
    // タイムアウト後にキーイベントが来る
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{middle_tp, trigger_key_2});
    state.push_event(KeyPressEvent{end_tp, trigger_key_3});
    REQUIRE_STATE_2(1, trigger_keyset_1, none_keyset, trigger_keyset_1);
  }
  SECTION("2KEYS-1: release K1") {
    // 第1キーを離す
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{middle_tp, trigger_key_2});
    state.push_event(KeyReleaseEvent{end_tp - 1ms, trigger_key_1});
    REQUIRE_STATE_2(1, trigger_keyset_1, none_keyset, trigger_keyset_1);
  }

  // 同時打鍵
  SECTION("2KEYS-2: timed out without next key") {
    // キーイベントが来ないままタイムアウトする
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{middle_tp - 1ms, trigger_key_2});
    REQUIRE_STATE_2(2, trigger_keyset_12, none_keyset, trigger_keyset_12);
  }
  SECTION("2KEYS-2: timed out with next key") {
    // タイムアウト後にキーイベントが来る
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{middle_tp - 1ms, trigger_key_2});
    state.push_event(KeyPressEvent{end_tp, trigger_key_3});
    REQUIRE_STATE_2(2, trigger_keyset_12, none_keyset, trigger_keyset_12);
  }
  SECTION("2KEYS-2: release K1") {
    // 第1キーを離す
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{middle_tp - 1ms, trigger_key_2});
    state.push_event(KeyReleaseEvent{end_tp - 1ms, trigger_key_1});
    REQUIRE_STATE_2(2, trigger_keyset_12, none_keyset, trigger_keyset_12);
  }

  // 単打
  SECTION("3KEYS-1:") {
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp - 15ms, trigger_key_1});
    state.push_event(KeyPressEvent{begin_tp, trigger_key_2});
    state.push_event(KeyPressEvent{begin_tp + 10ms, trigger_key_3});
    REQUIRE_STATE_3(1, trigger_keyset_1, none_keyset, trigger_keyset_1);
  }

  // 同時打鍵
  SECTION("3KEYS-2:") {
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp - 10ms, trigger_key_1});
    state.push_event(KeyPressEvent{begin_tp, trigger_key_2});
    state.push_event(KeyPressEvent{begin_tp + 15ms, trigger_key_3});
    REQUIRE_STATE_3(2, trigger_keyset_12, none_keyset, trigger_keyset_12);
  }

  SECTION("idle") {
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});
    state.push_event(KeyPressEvent{middle_tp, trigger_key_2});
    state.push_event(KeyPressEvent{end_tp, trigger_key_3});
    state.push_event(KeyReleaseEvent{begin_tp + timeout_dur - 1ms, trigger_key_1});

    flow.reset(state);
    REQUIRE(flow.is_idle(state) == false);
    flow.update(state);
    REQUIRE(flow.is_idle(state) == false);
    flow.update(state);
    REQUIRE(flow.is_idle(state) == false);
    flow.update(state);
    REQUIRE(flow.is_idle(state) == true);
  }

  SECTION("timeout_tp") {
    state.reset(config);
    state.push_event(KeyPressEvent{begin_tp, trigger_key_1});

    flow.reset(state);
    REQUIRE(flow.timeout_tp() == begin_tp + timeout_dur);
  }
}
