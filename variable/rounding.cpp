// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Samuel Jones

#include "scipp/core/element/rounding.h"
#include "scipp/variable/rounding.h"
#include "scipp/variable/transform.h"

Variable floor(const Variable &var) {
  return transform(var, scipp::core::element::floor, "floor");
}

Variable ceil(const Variable &var) {
  return transform(var, scipp::core::element::ceil, "ceil");
}

Variable round(const Variable &var, const scipp::index decimals) {
  return transform(
      var,
      [decimals](const auto &a) {
        return scipp::core::element::round(a, decimals);
      },
      "ceil");
}
