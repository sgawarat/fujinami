#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sol.hpp>
#include "../keyboard_config.hpp"
#include "errors.hpp"

namespace fujinami {
namespace config {
class LuaLoader final {
 public:
  LuaLoader();

  ~LuaLoader() noexcept;

  void load(KeyboardConfig& config);

 private:
  struct PassthroughHash final {
    using argument_type = size_t;
    using result_type = size_t;
    size_t operator()(size_t key) const noexcept { return key; }
  };
  using LayoutMap = std::unordered_map<size_t, std::shared_ptr<KeyboardLayout>,
                                       PassthroughHash>;

  void set_global_option(const sol::table& tbl);

  size_t get_layout_handle(const std::string& name);

  void create_flow(size_t layout_handle, int key, int flow_type);

  void create_mapping(size_t layout_handle, const sol::table& active_keys_tbl,
                      const sol::table& command_tbl);

  void create_next_layout(size_t layout_handle, const sol::table& keys_tbl,
                          const std::string& name);

  LayoutMap::const_iterator create_layout(const std::string& name);

  sol::state lua_;
  KeyboardConfig* config_ = nullptr;
  LayoutMap layout_map_;
  std::vector<Key> keys_;
  std::vector<KeyRole> roles_;
};
}  // namespace config
}  // namespace fujinami
