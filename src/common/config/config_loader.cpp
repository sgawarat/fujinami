#include <fujinami/config/loader.hpp>
#include <string>
#include <codecvt>
#include <fstream>
#include <sol.hpp>
#include <fujinami/platform.hpp>
#include <fujinami/keyboard_config.hpp>
#include <fujinami/config/errors.hpp>

namespace fujinami {
namespace config {
namespace {
// package_pathに列挙されたテンプレートを用いてスクリプトを読み込む。
sol::object load_script(sol::state_view& lua,
                        const std::string& package_path,
                        const std::vector<char>& mod_path) {
  const auto last = package_path.end();
  auto iter = package_path.begin();
  std::string path;
  while (true) {
    char c;
    if (iter == last) {
      c = ';';
    } else {
      c = *iter;
    }

    switch (c) {
      case ';': {
        if (!path.empty()) {
#if defined(FUJINAMI_PLATFORM_WIN32)
          std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
          const auto wpath = conv.from_bytes(path.data());
#elif defined(FUJINAMI_PLATFORM_LINUX)
          const auto& wpath = path;
#endif
          std::ifstream ifs(wpath);
          if (ifs.is_open()) {
            ifs.seekg(0, std::ios::end);
            const auto end_pos = ifs.tellg();
            ifs.clear();
            ifs.seekg(0, std::ios::beg);
            const auto beg_pos = ifs.tellg();
            const size_t len = end_pos - beg_pos;

            std::vector<char> code(len);
            if (len > 0) ifs.read(code.data(), code.size());
            const auto result = lua.script(std::string(code.data(), code.size()));
            if (result.valid()) return result.get<sol::object>();
          }

          path.clear();
        }
        if (iter == last) return sol::nil;
        break;
      }
      case '?': {
        path.insert(path.end(), mod_path.begin(), mod_path.end());
        break;
      }
      default: {
        path.push_back(c);
        break;
      }
    }
    ++iter;
  }

  assert("UNREACHABLE");
  return sol::nil;
}

// パス文字列をUnicodeに変換してからスクリプトを読み込む
sol::object wrequire(sol::this_state self, const std::string& modname) {
  sol::state_view lua(self.L);

  const auto& package = lua["package"];
  const auto& package_preload = package["preload"];

  // 読み込み済みオブジェクトが存在する場合、それを返す。
  auto preloaded_module = package_preload[modname];
  if (preloaded_module.valid()) return preloaded_module;

  // Luaのモジュール名をパス形式に変換する。
  std::vector<char> mod_path;
  mod_path.reserve(modname.size());
  for (char c : modname) {
    if (c == '.') {
      mod_path.push_back('/');
    } else {
      mod_path.push_back(c);
    }
  }

  const std::string& package_path = package["path"];

  // スクリプト読み込みが成功した場合、その戻り値を返す。
  auto ret = load_script(lua, package_path, mod_path);
  if (ret.valid()) {
    preloaded_module = ret;
    return ret;
  }

  // エラーメッセージを生成して、例外を投げる。
  std::string error_message = "module '" + modname + "' not found:\n\tno field package.preload['" + modname + "']\n";
  std::string path;
  for (const char c : package_path) {
    switch (c) {
      case ';': {
        if (!path.empty()) {
          error_message += "\tno file '" + path + "'\n";
          path.clear();
        }
        break;
      }
      case '?': {
        path.insert(path.end(), mod_path.begin(), mod_path.end());
        break;
      }
      default: {
        path.push_back(c);
        break;
      }
    }
  }
  if (!path.empty()) error_message += "\tno file '" + path + "'\n";

  throw sol::error(std::move(error_message));
}
}  // namespace

LuaLoader::LuaLoader() {}

LuaLoader::~LuaLoader() noexcept {}

void LuaLoader::load(KeyboardConfig& config) {
  config_ = &config;
  layout_map_.clear();
  keys_.clear();
  roles_.clear();

  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine,
                     sol::lib::string, sol::lib::os, sol::lib::math,
                     sol::lib::table, sol::lib::debug, sol::lib::bit32,
                     sol::lib::io, sol::lib::jit, sol::lib::utf8);

  lua.set_function("wrequire", &wrequire);

#if defined(FUJINAMI_PLATFORM_WIN32)
  lua.new_enum("Platform", "NAME", "win32",
                "WIN32", true, "LINUX", false);
#elif defined(FUJINAMI_PLATFORM_LINUX)
  lua.new_enum("Platform", "NAME", "linux",
                "WIN32", false, "LINUX", true);
#endif

  lua.new_enum(
      "Mod", "SHIFT", int(Modifier::SHIFT_LEFT), "CONTROL",
      int(Modifier::CONTROL_LEFT), "ALT", int(Modifier::ALT_LEFT), "OS",
      int(Modifier::OS_LEFT), "SHIFT_LEFT", int(Modifier::SHIFT_LEFT),
      "SHIFT_RIGHT", int(Modifier::SHIFT_RIGHT), "CONTROL_LEFT",
      int(Modifier::CONTROL_LEFT), "CONTROL_RIGHT",
      int(Modifier::CONTROL_RIGHT), "ALT_LEFT", int(Modifier::ALT_LEFT),
      "ALT_RIGHT", int(Modifier::ALT_RIGHT), "OS_LEFT", int(Modifier::OS_LEFT),
      "OS_RIGHT", int(Modifier::OS_RIGHT), "OS_RIGHT", int(Modifier::OS_RIGHT),
      "ALL", int(Modifier::ALL));

  lua.new_enum("FlowType", "IMMEDIATE", int(FlowType::IMMEDIATE), "DEFERRED",
                int(FlowType::DEFERRED), "SIMUL", int(FlowType::SIMUL));

  lua.new_enum("KeyRole", "TRIGGER", int(KeyRole::TRIGGER), "MODIFIER",
                int(KeyRole::MODIFIER));

  auto impl_table = lua.create_named_table("fujinami");
  impl_table.set_function("set_global_option", &LuaLoader::set_global_option,
                          this);
  impl_table.set_function("get_layout_handle", &LuaLoader::get_layout_handle,
                          this);
  impl_table.set_function("create_flow", &LuaLoader::create_flow, this);
  impl_table.set_function("create_mapping", &LuaLoader::create_mapping, this);
  impl_table.set_function("create_next_layout", &LuaLoader::create_next_layout,
                          this);

  const std::string package_path = lua["package"]["path"];
  lua["package"]["path"] = package_path
      + ";./config/?.lua"
      + ";./config/?/init.lua";

  const auto result = lua.script_file("./fujinami.lua");
  if (!result.valid()) {
    throw LoaderError("failed to load a config file");
  }
}

size_t LuaLoader::get_layout_handle(const std::string& name) {
  if (!config_) throw LoaderError("config is null");
  return create_layout(name)->first;
}

void LuaLoader::set_global_option(const sol::table& tbl) {
  if (!config_) throw LoaderError("config is null");

  auto timeout_ms_opt = tbl.get<sol::optional<int>>("timeout_milliseconds");
  if (timeout_ms_opt) {
    config_->set_timeout_dur(std::chrono::milliseconds(*timeout_ms_opt));
  }
  auto default_layout_opt =
      tbl.get<sol::optional<std::string>>("default_layout");
  if (default_layout_opt) {
    config_->set_default_layout(create_layout(*default_layout_opt)->second);
  }
  auto default_im_layout_opt =
      tbl.get<sol::optional<std::string>>("default_im_layout");
  if (default_im_layout_opt) {
    config_->set_default_im_layout(create_layout(*default_im_layout_opt)->second);
  }

  auto auto_layout_opt = tbl.get<sol::optional<bool>>("auto_layout");
  if (auto_layout_opt) {
    config_->set_auto_layout(*auto_layout_opt);
  }
}

void LuaLoader::create_flow(size_t layout_handle, int key, int flow_type) {
  auto iter = layout_map_.find(layout_handle);
  if (iter == layout_map_.end()) throw LoaderError("invalid layout handle");
  if (key < 0 || key > KEY_COUNT) throw LoaderError("invalid key");
  if (flow_type < int(FlowType::UNKNOWN) ||
      flow_type > int(FlowType::SIMUL)) {
    throw LoaderError("invalid flow_type");
  }
  if (iter->second && key != 0) {
    iter->second->create_flow(static_cast<Key>(key), static_cast<FlowType>(flow_type));
  }
}

void LuaLoader::create_mapping(size_t layout_handle,
                               const sol::table& active_keys_tbl,
                               const sol::table& command_tbl) {
  auto iter = layout_map_.find(layout_handle);
  if (iter == layout_map_.end()) throw LoaderError("invalid layout handle");
  keys_.clear();
  roles_.clear();
  active_keys_tbl.for_each(
      [&](const sol::object& i, const sol::table& active_key) {
        const int key = active_key.get_or(1, 0);
        const int role = active_key.get_or(2, 0);
        if (key < 0 || key > KEY_COUNT) throw LoaderError("invalid key");
        if (role < int(KeyRole::NONE) || role > int(KeyRole::MODIFIER)) {
          throw LoaderError("invalid key_role");
        }
        if (key != 0) {
          keys_.push_back(static_cast<Key>(key));
          roles_.push_back(static_cast<KeyRole>(role));
        }
      });

  Command command;
  command_tbl.for_each([&](const sol::object& i, const sol::table& any_action_tbl) {
    if (!any_action_tbl.empty()) {
      const auto action_type = any_action_tbl.get<sol::object>(1).get_type();
      switch (any_action_tbl.get<sol::object>(1).get_type()) {
        case sol::type::number: {
          const int key = any_action_tbl.get<int>(1);
          const int modifiers = any_action_tbl.get_or(2, 0);
          if (key < 0 || key > KEY_COUNT) throw LoaderError("invalid key");
          if (modifiers < 0 || modifiers > int(Modifier::ALL)) {
            throw LoaderError("invalid modifiers");
          }
          command.emplace_back(
              KeyAction(static_cast<Key>(key), static_cast<Modifier>(modifiers)));
          break;
        }
        case sol::type::string: {
          const auto char_action_str = any_action_tbl.get<std::string>(1);
#ifdef _MSC_VER
          std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> conv;
#else
          std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
#endif
          auto str = conv.from_bytes(char_action_str);
          for (auto c : str) {
            command.emplace_back(CharAction(static_cast<char16_t>(c)));
          }
          break;
        }
        default:
          throw LoaderError("invalid action");
      }
    }
  });

  if (iter->second && !keys_.empty() && !command.is_empty()) {
    iter->second->create_mapping(keys_, roles_, std::move(command));
  }
}

void LuaLoader::create_next_layout(size_t layout_handle,
                                   const sol::table& keys_tbl,
                                   const std::string& name) {
  auto iter = layout_map_.find(layout_handle);
  if (iter == layout_map_.end()) throw LoaderError("invalid layout handle");
  Keyset keyset;
  keys_tbl.for_each([&](const sol::object& i, const sol::object& key_obj) {
    int key = key_obj.as<int>();
    if (key < 0 || key > KEY_COUNT) throw LoaderError("invalid key");
    if (key != 0) keyset += static_cast<Key>(key);
  });

  if (iter->second && keyset && !name.empty()) {
    iter->second->create_transition(keyset, create_layout(name)->second);
  }
}

LuaLoader::LayoutMap::const_iterator LuaLoader::create_layout(const std::string& name) {
  if (name.empty()) throw LoaderError("invalid layout name");

  const size_t layout_handle = std::hash<std::string>{}(name);
  auto iter = layout_map_.find(layout_handle);
  if (iter != layout_map_.end()) return iter;

  auto layout = config_->create_layout(name);
  return layout_map_.emplace(layout_handle, layout).first;
}
}  // namespace config
}  // namespace fujinami
