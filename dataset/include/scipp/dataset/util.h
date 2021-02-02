// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew
#pragma once
#include <scipp/dataset/dataset.h>
#include <scipp/variable/variable.h>

#include "scipp/dataset/generated_util.h"

namespace scipp {
SCIPP_DATASET_EXPORT scipp::index size_of(const VariableConstView &view);
SCIPP_DATASET_EXPORT scipp::index size_of(const DataArrayConstView &dataarray,
                                          bool include_aligned_coords = true);
SCIPP_DATASET_EXPORT scipp::index size_of(const DatasetConstView &dataset);
} // namespace scipp
