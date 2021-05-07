// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew
#pragma once
#include <scipp/dataset/dataset.h>
#include <scipp/variable/variable.h>

#include "scipp/dataset/generated_util.h"

namespace scipp {

SCIPP_DATASET_EXPORT scipp::index size_of(const Variable &view);
SCIPP_DATASET_EXPORT scipp::index size_of(const DataArray &dataarray,
                                          bool include_aligned_coords = true);
SCIPP_DATASET_EXPORT scipp::index size_of(const Dataset &dataset);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray astype(const DataArray &var,
                                                    const DType type);

} // namespace scipp
