// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew
#pragma once
#include <scipp/dataset/dataset.h>
#include <scipp/variable/variable.h>

namespace scipp {
SCIPP_DATASET_EXPORT scipp::index size_of(const VariableConstView &view);
SCIPP_DATASET_EXPORT scipp::index size_of(const DataArrayConstView &dataarray,
                                          bool include_aligned_coords = true);
SCIPP_DATASET_EXPORT scipp::index size_of(const DatasetConstView &dataset);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
values(const DataArrayConstView &x);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
variances(const DataArrayConstView &x);

} // namespace scipp
