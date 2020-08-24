// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Thibault Chatel
#include <numeric>

#include "scipp/core/element/sort.h"
#include "scipp/variable/sort.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable sort(const VariableConstView &var, const Dim dim,
              const SortOrder order) {
  Variable out(var);
  if (order == SortOrder::Ascending)
    transform_in_place(subspan_view(out, dim),
                       core::element::sort_nondescending);
  else
    transform_in_place(subspan_view(out, dim),
                       core::element::sort_nonascending);
  return out;
}

} // namespace scipp::variable
