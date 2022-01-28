// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/math.h"

#include "scipp/core/element/math.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

using MidpointTypes =
    std::tuple<double, float, int32_t, int64_t, core::time_point>;

Variable midpoints(const Variable &var, const std::optional<Dim> dim) {
  if (var.ndim() == 0) {
    if (dim.has_value()) {
      throw except::DimensionError(
          "Called `midpoints` with a scalar but specified dimension `" +
          to_string(*dim) + "`.");
    }
    return var;
  }

  if (!dim.has_value() && var.ndim() != 1) {
    throw std::invalid_argument("Cannot deduce dimension to compute "
                                "midpoints of variable with dimensions " +
                                to_string(var.dims()) +
                                ". Select one using the `dim` argument.");
  }

  const auto d = dim.has_value() ? *dim : var.dim();
  const auto len = var.dims()[d];
  if (len == scipp::index{1}) {
    return var;
  }
  return transform<MidpointTypes>(var.slice({d, 0, len - 1}),
                                  var.slice({d, 1, len}),
                                  core::element::midpoint, "midpoints");
}
} // namespace scipp::variable
