#pragma once

#include <stdexcept>
#include <gsl/gsl>
#include <spdlog/fmt/fmt.h>
#include "../logging.hpp"

namespace fujinami {
namespace config {
class LoaderError : public std::runtime_error {
 public:
  LoaderError() = default;

  LoaderError(const std::string& s) : runtime_error(s) {}

  template <typename T, typename... Others>
  LoaderError(const std::string& s, T&& a0, Others&&... others) noexcept
      : LoaderError(fmt::format(s, std::forward<T>(a0),
                                std::forward<Others>(others)...)) {}
};
}  // namespace config
}  // namespace fujinami
