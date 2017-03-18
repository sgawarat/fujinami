#pragma once

#include <cstdint>
#include <new>
#include <type_traits>
#include <vector>
#include "logging.hpp"
#include "platform.hpp"
#if defined(FUJINAMI_PLATFORM_WIN32)
#include <fujinami_win32/action.hpp>
#elif defined(FUJINAMI_PLATFORM_LINUX)
#include <fujinami_linux/action.hpp>
#endif

namespace fujinami {
class AnyAction {
 public:
  static constexpr bool is_nothrow_copy_constructible =
      std::is_nothrow_copy_constructible<KeyAction>::value &&
      std::is_nothrow_copy_constructible<CharAction>::value;
  static constexpr bool is_nothrow_copy_assignable =
      is_nothrow_copy_constructible &&
      std::is_nothrow_copy_assignable<KeyAction>::value &&
      std::is_nothrow_copy_assignable<CharAction>::value;

  AnyAction() noexcept : type_(Type::NONE) {}

  AnyAction(const AnyAction& other) noexcept(is_nothrow_copy_constructible) {
    construct(other);
  }

  AnyAction(AnyAction&& other) noexcept { construct(std::move(other)); }

  AnyAction(const KeyAction& action) noexcept(
      std::is_nothrow_copy_constructible<KeyAction>::value) {
    new (&key_) KeyAction(action);
    type_ = Type::KEY;
  }

  AnyAction(KeyAction&& action) noexcept {
    new (&key_) KeyAction(std::move(action));
    type_ = Type::KEY;
  }

  AnyAction(const CharAction& action) noexcept(
      std::is_nothrow_copy_constructible<CharAction>::value) {
    new (&char_) CharAction(action);
    type_ = Type::CHAR;
  }

  AnyAction(CharAction&& action) noexcept {
    new (&char_) CharAction(std::move(action));
    type_ = Type::CHAR;
  }

  ~AnyAction() noexcept { destruct(); }

  AnyAction& operator=(const AnyAction& other) noexcept(
      is_nothrow_copy_assignable) {
    if (type_ == other.type_) {
      switch (type_) {
        case Type::KEY:
          key_ = other.key_;
          break;
        case Type::CHAR:
          char_ = other.char_;
          break;
      }
    } else {
      destruct();
      type_ = Type::NONE;
      construct(other);
    }
    return *this;
  }

  AnyAction& operator=(AnyAction&& other) noexcept {
    if (type_ == other.type_) {
      switch (type_) {
        case Type::KEY:
          key_ = std::move(other.key_);
          break;
        case Type::CHAR:
          char_ = std::move(other.char_);
          break;
      }
    } else {
      destruct();
      type_ = Type::NONE;
      construct(std::move(other));
    }
    return *this;
  }

  void press(const AnyAction& prev) const noexcept {
    switch (type_) {
      case Type::KEY: {
        if (prev.type_ == Type::KEY) {
          key_.press(prev.key_);
        } else {
          prev.release();
          key_.press();
        }
        break;
      }
      case Type::CHAR: {
        if (prev.type_ == Type::CHAR) {
          char_.press(prev.char_);
        } else {
          prev.release();
          char_.press();
        }
        break;
      }
    }
  }

  void press() const noexcept {
    switch (type_) {
      case Type::KEY: {
        key_.press();
        break;
      }
      case Type::CHAR: {
        char_.press();
        break;
      }
    }
  }

  void repeat(const AnyAction& prev) const noexcept {
    switch (type_) {
      case Type::KEY: {
        if (prev.type_ == Type::KEY) {
          key_.repeat(prev.key_);
        } else {
          prev.release();
          key_.repeat();
        }
        break;
      }
      case Type::CHAR: {
        if (prev.type_ == Type::CHAR) {
          char_.repeat(prev.char_);
        } else {
          prev.release();
          char_.repeat();
        }
        break;
      }
    }
  }

  void repeat() const noexcept {
    switch (type_) {
      case Type::KEY: {
        key_.repeat();
        break;
      }
      case Type::CHAR: {
        char_.repeat();
        break;
      }
    }
  }

  void release() const noexcept {
    switch (type_) {
      case Type::KEY: {
        key_.release();
        break;
      }
      case Type::CHAR: {
        char_.release();
        break;
      }
    }
  }

  FUJINAMI_LOGGING_UNION(AnyAction, Type,
                         ((KEY, "key", key_))((CHAR, "char", char_)));

 private:
  enum class Type : uint8_t {
    NONE,
    KEY,
    CHAR,
  };

  void construct(const AnyAction& other) noexcept(
      is_nothrow_copy_constructible) {
    switch (other.type_) {
      case Type::KEY:
        new (&key_) KeyAction(other.key_);
        break;
      case Type::CHAR:
        new (&char_) CharAction(other.char_);
        break;
    }
    type_ = other.type_;
  }

  void construct(AnyAction&& other) noexcept {
    switch (other.type_) {
      case Type::KEY:
        new (&key_) KeyAction(std::move(other.key_));
        break;
      case Type::CHAR:
        new (&char_) CharAction(std::move(other.char_));
        break;
    }
    type_ = other.type_;
  }

  void destruct() noexcept {
    switch (type_) {
      case Type::KEY:
        key_.~KeyAction();
        break;
      case Type::CHAR:
        char_.~CharAction();
        break;
    }
  }

  Type type_ = Type::NONE;
  union {
    KeyAction key_;
    CharAction char_;
  };
};

class Command final {
 public:
  void press(const Command* prev) const noexcept {
    if (actions_.empty()) {
      if (prev) prev->release();
    } else {
      if (!prev || prev->actions_.empty()) {
        actions_.front().press();
      } else {
        actions_.front().press(prev->actions_.back());
      }
      for (size_t i = 1; i < actions_.size(); ++i) {
        actions_[i].press(actions_[i - 1]);
      }
    }
  }

  void repeat(const Command* prev) const noexcept {
    if (actions_.empty()) {
      if (prev) prev->release();
    } else {
      if (!prev || prev->actions_.empty()) {
        actions_.front().repeat();
      } else {
        actions_.front().repeat(prev->actions_.back());
      }
      for (size_t i = 1; i < actions_.size(); ++i) {
        actions_[i].press(actions_[i - 1]);
      }
    }
  }

  void release() const noexcept {
    if (!actions_.empty()) actions_.back().release();
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    actions_.emplace_back(std::forward<Args>(args)...);
  }

  void insert(const Command& command) {
    actions_.insert(actions_.end(), command.actions_.begin(),
                    command.actions_.end());
  }

  void insert(Command&& command) {
    actions_.insert(actions_.end(),
                    std::make_move_iterator(command.actions_.begin()),
                    std::make_move_iterator(command.actions_.end()));
    command.actions_.clear();
  }

  bool is_empty() const noexcept { return actions_.empty(); }

  FUJINAMI_LOGGING_STRUCT(Command, (("actions", actions_)));

 private:
  std::vector<AnyAction> actions_;
};
}  // namespace fujinami
