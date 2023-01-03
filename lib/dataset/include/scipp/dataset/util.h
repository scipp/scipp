// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew
#pragma once

#include "scipp/variable/variable.h"

#include "scipp-dataset_export.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/generated_util.h"

namespace scipp {

enum SizeofTag { Underlying, ViewOnly };

SCIPP_DATASET_EXPORT scipp::index size_of(const Variable &var, SizeofTag tag);
SCIPP_DATASET_EXPORT scipp::index size_of(const DataArray &da, SizeofTag tag,
                                          bool include_aligned_coords = true);
SCIPP_DATASET_EXPORT scipp::index size_of(const Dataset &ds, SizeofTag tag);
} // namespace scipp

namespace scipp::dataset {

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
strip_edges_along(const DataArray &da, const Dim dim);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset strip_edges_along(const Dataset &ds,
                                                             const Dim dim);

} // namespace scipp::dataset
