// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <optional>
#include <string>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "scipp/units/unit.h"

#include "scipp/core/eigen.h"
#include "scipp/core/format.h"
#include "scipp/core/string.h"

namespace scipp::core {
template <class T>
std::string array_to_string(
    const T &arr,
    [[maybe_unused]] const std::optional<units::Unit> &unit = std::nullopt) {
  const auto &formatters = core::FormatRegistry::instance();
  const auto size = scipp::size(arr);
  if (size == 0)
    return "[]";
  std::string s = "[";
  for (scipp::index i = 0; i < size; ++i) {
    constexpr scipp::index n = 2;
    if (i == n && size > 2 * n) {
      s += "..., ";
      i = size - n;
    }
    s += formatters.format(arr[i], core::FormatSpec{"", unit}) + ", ";
  }
  s.resize(s.size() - 2);
  s += "]";
  return s;
}

} // namespace scipp::core
