// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/util.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/except.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/transform_subspan.h"

using namespace scipp::core;

namespace scipp::variable {

Variable linspace(const VariableConstView &start, const VariableConstView &stop,
                  const Dim dim, const scipp::index num) {
  core::expect::equals(start.dims(), stop.dims());
  core::expect::equals(start.unit(), stop.unit());
  auto dims = start.dims();
  dims.addInner(dim, num);
  Variable out = broadcast(start, dims);
  const auto range = stop - start;
  for (scipp::index i = 1; i < num - 1; ++i)
    out.slice({dim, i}) += (static_cast<double>(i) / num * units::one) * range;
  out.slice({dim, num - 1}).assign(stop); // endpoint included
  return out;
}

} // namespace scipp::variable
