// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew
#pragma once
#include <scipp/dataset/dataset.h>
#include <scipp/variable/variable.h>

namespace scipp {
SCIPP_DATASET_EXPORT scipp::index size_of(const VariableConstView &view);

/// Return the size in memory of a DataArray object. The aligned coord is
/// optional becuase for a DataArray owned by a dataset aligned coords are
/// assumed to be owned by the dataset as they can apply to multiple arrays.
SCIPP_DATASET_EXPORT scipp::index size_of(const DataArrayConstView &dataarray,
                                          bool include_aligned_coords=false);
SCIPP_DATASET_EXPORT scipp::index size_of(const DatasetConstView &dataset);
} // namespace scipp