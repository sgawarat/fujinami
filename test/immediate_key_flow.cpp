#include <catch.hpp>
#include <fujinami/buffering/flow/immediate.hpp>

using namespace fujinami;
using namespace fujinami::buffering;

TEST_CASE("ImmediateKeyFlow", "[fujinami][flow]") {
  const Key key = to_key(VK_PRINT, false);
  const KeyRole key_role = KeyRole::TRIGGER;
  KeyboardConfig config;
  auto unmapped_layout = config.create_layout("unmapped");
  auto mapped_layout = config.create_layout("mapped");
  mapped_layout->create_flow(key, FlowType::IMMEDIATE);
  mapped_layout->create_mapping({&key, 1}, {&key_role, 1}, Command{});

  SECTION("reset with unmapped layout") {
    State state;
    state.reset(unmapped_layout);
    state.push_event(KeyPressEvent{Clock::now(), key});

    ImmediateKeyFlow flow;
    REQUIRE(flow.reset(state) == FlowResult::DONE);
    REQUIRE(state.events().empty());
    REQUIRE(state.trigger_keyset() == Keyset{});
    REQUIRE(state.dontcare_keyset() == Keyset{key});
  }

  SECTION("reset with mapped layout") {
    State state;
    state.reset(mapped_layout);
    state.push_event(KeyPressEvent{Clock::now(), key});

    ImmediateKeyFlow flow;
    REQUIRE(flow.reset(state) == FlowResult::DONE);
    REQUIRE(state.events().empty());
    REQUIRE(state.trigger_keyset() == Keyset{key});
    REQUIRE(state.dontcare_keyset() == Keyset{key});
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
