// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/map_to_bins.h"

#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

#include "bin_detail.h"

namespace scipp::dataset::bin_detail {
/// Implementation detail of dataset::bin
void map_to_bins(Variable &out, const Variable &var, const Variable &offsets,
                 const Variable &indices) {
  transform_in_place(out, offsets, var, indices, core::element::bin, "bin");
}
} // namespace scipp::dataset::bin_detail
