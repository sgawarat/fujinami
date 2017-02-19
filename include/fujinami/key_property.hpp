﻿#pragma once

#include "flagset.hpp"
#include "logging.hpp"

namespace fujinami {
class KeyPropertyMap;

enum class FlowType : uint8_t {
  UNKNOWN,
  IMMEDIATE,
  DEFERRED,
};
FUJINAMI_LOGGING_ENUM(inline, FlowType,
                      (UNKNOWN)(IMMEDIATE)(DEFERRED));

class KeyProperty final {
 public:
  KeyProperty() = default;

  explicit KeyProperty(FlowType flow_type) noexcept : flow_type_(flow_type) {}

  FlowType flow_type() const noexcept { return flow_type_; }

  FUJINAMI_LOGGING_STRUCT(KeyProperty, (("flow_type", flow_type_)));

 private:
  FlowType flow_type_ = FlowType::UNKNOWN;
};
}  // namespace fujinami
