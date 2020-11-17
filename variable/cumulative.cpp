// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/cumulative.h"
#include "scipp/core/element/cumulative.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

using namespace scipp::core;

namespace scipp::variable {

/// In-place std::exclusive_scan along dim.
void exclusive_scan(const VariableView &var, const Dim dim) {
  // transform_in_place(subspan_view(var, dim), core::element::exclusive_scan);
  Variable sum(var.slice({dim, 0}));
  fill_zeros(var.slice({dim, 0}));
  for (scipp::index i = 1; i < var.dims()[dim]; ++i) {
    const auto &current = var.slice({dim, i});
    sum += current;
    current *= makeVariable<int32_t>(Values{-1});
    current += sum;
  }
}

} // namespace scipp::variable
