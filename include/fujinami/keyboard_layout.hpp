#pragma once

#include <gsl/gsl>
#include "logging.hpp"
#include "command.hpp"
#include "key_property.hpp"
#include "keyset_property.hpp"

namespace fujinami {
enum class KeyRole : uint8_t {
  NONE,
  TRIGGER,
  MODIFIER,
};
FUJINAMI_LOGGING_ENUM(inline, KeyRole, (NONE)(TRIGGER)(MODIFIER));

class KeyboardLayout final {
 public:
  using ActiveKeyMask = uint64_t;
  static constexpr size_t MAX_ACTIVE_KEY_COUNT = sizeof(ActiveKeyMask) * 8;

  explicit KeyboardLayout(const std::string& name) : name_(name) {
    keyset_property_map_.reserve(10240);
  }

  ~KeyboardLayout() noexcept {}

  void reset() {
    inserted_key_property_bits_.reset();
    keyset_property_map_.clear();
    command_map_.clear();
    next_layout_map_.clear();
  }

  bool create_flow(Key key, FlowType flow_type) {
    if (inserted_key_property_bits_[key]) return false;
    inserted_key_property_bits_ += key;
    key_properties_[static_cast<size_t>(key)] = KeyProperty(flow_type);
    return true;
  }

  bool create_mapping(gsl::span<const Key> keys, gsl::span<const KeyRole> roles,
                      Command&& command) {
    if (keys.size() != roles.size()) {
      throw std::invalid_argument("keys size != roles size");
    }
    if (keys.size() >= MAX_ACTIVE_KEY_COUNT) {
      throw std::logic_error("too much active keys");
    }

    Keyset active_keyset;
    Keyset trigger_keyset;
    Keyset modifier_keyset;
    for (decltype(keys.size()) i = 0; i < keys.size(); ++i) {
      active_keyset += keys[i];
      switch (roles[i]) {
        case KeyRole::TRIGGER:
          trigger_keyset += keys[i];
          break;
        case KeyRole::MODIFIER:
          modifier_keyset += keys[i];
          break;
      }
    }

    // FUJINAMI_LOG(debug, "new mapping (active_keyset:{}, command:{})",
    // active_keyset, command);

    if (!command_map_.emplace(active_keyset, std::move(command)).second) {
      // FUJINAMI_LOG(info, "mapping already exists (active_keyset:{})",
      // active_keyset);
      return false;
    }

    map(keys);
    keyset_property_map_[active_keyset].make_mapped(trigger_keyset,
                                                    modifier_keyset);
    return true;
  }

  bool create_transition(
      const Keyset& active_keyset,
      const std::shared_ptr<const KeyboardLayout>& next_layout) {
    if (!next_layout) return false;
    return next_layout_map_.emplace(active_keyset, next_layout).second;
  }

  const KeyProperty* find_key_property(Key key) const noexcept {
    if (!inserted_key_property_bits_[key]) return nullptr;
    return &key_properties_[static_cast<size_t>(key)];
  }

  const KeysetProperty* find_keyset_property(const Keyset& keyset) const
      noexcept {
    auto iter = keyset_property_map_.find(keyset);
    if (iter == keyset_property_map_.end()) return nullptr;
    return &iter->second;
  }

  const Command* find_command(const Keyset& keyset) const noexcept {
    const auto iter = command_map_.find(keyset);
    if (iter == command_map_.end()) return nullptr;
    return &iter->second;
  }

  std::weak_ptr<const KeyboardLayout> find_next_layout(
      const Keyset& keyset) const noexcept {
    const auto iter = next_layout_map_.find(keyset);
    if (iter == next_layout_map_.end()) return {};
    return iter->second;
  }

  gsl::czstring name() const noexcept { return name_.c_str(); }

 private:
  // 組み合わせ可能なキーセットを設定する。
  void map(gsl::span<const Key> keys) {
    // nodes
    const ActiveKeyMask all_mask = (ActiveKeyMask(1) << keys.size()) - 1;
    for (ActiveKeyMask mask = 1; mask != all_mask; ++mask) {
      Keyset active_keyset;
      Keyset combinable_keyset;
      for (decltype(keys.size()) i = 0; i < keys.size(); ++i) {
        if (mask & (ActiveKeyMask(1) << i)) {
          active_keyset += keys[i];
        } else {
          combinable_keyset += keys[i];
        }
      }
      keyset_property_map_[active_keyset].make_node(combinable_keyset);
    }
  }

  std::string name_;
  Keyset inserted_key_property_bits_;
  std::array<KeyProperty, KEY_COUNT> key_properties_;
  std::unordered_map<Keyset, KeysetProperty> keyset_property_map_;
  std::unordered_map<Keyset, Command> command_map_;
  std::unordered_map<Keyset, std::weak_ptr<const KeyboardLayout>>
      next_layout_map_;
};

FUJINAMI_LOGGING_DEFINE_PRINT(inline, std::shared_ptr<const KeyboardLayout>,
                              layout_ptr, (if (layout_ptr) {
                                os << layout_ptr->name();
                              } else { os << "null"; }));
}  // namespace fujinami
