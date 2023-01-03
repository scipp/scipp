// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Thibault Chatel
#include "scipp/core/element/sort.h"
#include "scipp/variable/sort.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

Variable sort(const Variable &var, const Dim dim, const SortOrder order) {
  auto out = copy(var);
  if (order == SortOrder::Ascending)
    transform_in_place(subspan_view(out, dim),
                       core::element::sort_nondescending, "sort");
  else
    transform_in_place(subspan_view(out, dim), core::element::sort_nonascending,
                       "sort");
  return out;
}

} // namespace scipp::variable
