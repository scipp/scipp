// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#pragma once

#include <string>

#include "scipp/core/format.h"
#include "scipp/core/sizes.h"

#include "scipp-variable_export.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {
struct SCIPP_VARIABLE_EXPORT VariableFormatSpec {
  bool show_type = true;
  std::optional<core::Sizes> container_sizes = std::nullopt;
  core::FormatSpec nested{};

  [[nodiscard]] VariableFormatSpec with_show_type(const bool value) const {
    auto res = *this;
    res.show_type = value;
    return res;
  }

  [[nodiscard]] VariableFormatSpec
  with_container_sizes(const std::optional<core::Sizes> &value) const {
    auto res = *this;
    res.container_sizes = value;
    return res;
  }

  [[nodiscard]] VariableFormatSpec
  with_nested(const core::FormatSpec &value) const {
    auto res = *this;
    res.nested = value;
    return res;
  }
};

std::string SCIPP_VARIABLE_EXPORT
format_variable(const Variable &var, const VariableFormatSpec &spec,
                const core::FormatRegistry &formatters);
} // namespace scipp::variable
