// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/math.h"

#include "scipp/core/element/math.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

Variable midpoints(const Variable &var, const std::optional<Dim> dim) {
  if (var.ndim() == 0) {
    throw except::DimensionError(
        "`midpoints` requires at least one input dimension, got a scalar.");
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
    throw except::DimensionError("Cannot compute midpoints in dimension `" +
                                 to_string(d) + "` of length 1.");
  }
  return transform(var.slice({d, 0, len - 1}), var.slice({d, 1, len}),
                   core::element::midpoint, "midpoints");
} // namespace scipp::variable
} // namespace scipp::variable
