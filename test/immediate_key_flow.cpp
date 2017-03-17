#include <catch.hpp>
#include <fujinami/buffering/flow/immediate.hpp>

using namespace std::chrono_literals;
using namespace fujinami;
using namespace fujinami::buffering;

#define REQUIRE_STATE(t, m, dc)\
  do {\
    const size_t size = state.events().size();\
    REQUIRE(flow.reset(state) == FlowResult::DONE);\
    REQUIRE(size == state.events().size() + 1);\
    REQUIRE(state.trigger_keyset() == t);\
    REQUIRE(state.modifier_keyset() == m);\
    REQUIRE(state.dontcare_keyset() == dc);\
  } while(false)

TEST_CASE("ImmediateKeyFlow", "[fujinami][buffering][flow]") {
  const Key trigger_key = to_key(1);
  const Key modifier_key = to_key(21);
  const Key unmapped_key = to_key(42);
  const KeyRole trigger_key_role = KeyRole::TRIGGER;
  const KeyRole modifier_key_role = KeyRole::MODIFIER;
  const Keyset none_keyset{};
  const Keyset trigger_keyset{trigger_key};
  const Keyset modifier_keyset{modifier_key};
  const Keyset all_keyset{trigger_key, modifier_key};

  auto config = std::make_shared<KeyboardConfig>();
  auto layout = config->create_layout("ImmediateKeyFlowTest");
  config->set_default_layout(layout);
  layout->create_flow(trigger_key, FlowType::IMMEDIATE);
  layout->create_flow(modifier_key, FlowType::IMMEDIATE);
  layout->create_mapping({trigger_key}, {trigger_key_role}, Command{});
  layout->create_mapping({modifier_key}, {modifier_key_role}, Command{});
  layout->create_mapping({trigger_key, modifier_key},
                         {trigger_key_role, modifier_key_role},
                         Command{});

  SECTION("press unmapped key") {
    ImmediateKeyFlow flow;
    State state;
    state.reset(config);

    // 0: Up Up Up
    state.push_event(KeyPressEvent{Clock::time_point(0ms), unmapped_key});
    state.push_event(KeyPressEvent{Clock::time_point(2ms), unmapped_key});
    state.push_event(KeyPressEvent{Clock::time_point(3ms), unmapped_key});
    REQUIRE_STATE(none_keyset, none_keyset, Keyset{unmapped_key});
    REQUIRE_STATE(none_keyset, none_keyset, Keyset{unmapped_key});
    REQUIRE_STATE(none_keyset, none_keyset, Keyset{unmapped_key});
  }

  SECTION("press trigger key") {
    ImmediateKeyFlow flow;
    State state;

    // 0: Tp Tp Tp
    state.reset(config);
    state.push_event(KeyPressEvent{Clock::time_point(0ms), trigger_key});
    state.push_event(KeyPressEvent{Clock::time_point(2ms), trigger_key});
    state.push_event(KeyPressEvent{Clock::time_point(3ms), trigger_key});
    REQUIRE_STATE(trigger_keyset, none_keyset, trigger_keyset);
    REQUIRE_STATE(trigger_keyset, none_keyset, trigger_keyset);
    REQUIRE_STATE(trigger_keyset, none_keyset, trigger_keyset);

    // 1: Mp Mp Mp
    state.reset(config);
    state.push_event(KeyPressEvent{Clock::time_point(100ms), modifier_key});
    state.push_event(KeyPressEvent{Clock::time_point(102ms), modifier_key});
    state.push_event(KeyPressEvent{Clock::time_point(103ms), modifier_key});
    REQUIRE_STATE(none_keyset, modifier_keyset, modifier_keyset);
    REQUIRE_STATE(none_keyset, modifier_keyset, modifier_keyset);
    REQUIRE_STATE(none_keyset, modifier_keyset, modifier_keyset);

    // 2: Tp Mp
    state.reset(config);
    state.push_event(KeyPressEvent{Clock::time_point(200ms), trigger_key});
    state.push_event(KeyPressEvent{Clock::time_point(201ms), modifier_key});
    REQUIRE_STATE(trigger_keyset, none_keyset, trigger_keyset);
    REQUIRE_STATE(none_keyset, modifier_keyset, all_keyset);

    // 3: Mp Tp
    state.reset(config);
    state.push_event(KeyPressEvent{Clock::time_point(300ms), modifier_key});
    state.push_event(KeyPressEvent{Clock::time_point(301ms), trigger_key});
    REQUIRE_STATE(none_keyset, modifier_keyset, modifier_keyset);
    REQUIRE_STATE(trigger_keyset, modifier_keyset, all_keyset);
  }

  SECTION("is_idle") {
    State state;
    ImmediateKeyFlow flow;
    REQUIRE(flow.is_idle(state) == true);
  }

  SECTION("update") {
    // ImmediateKeyFlow::update is unreachable
    //State state;
    //ImmediateKeyFlow flow;
    //REQUIRE(flow.update() == FlowResult::DONE);
  }
}
