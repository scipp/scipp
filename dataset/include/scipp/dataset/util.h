// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Matthew Andrew
#pragma once

#include "scipp/variable/variable.h"

#include "scipp-dataset_export.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/generated_util.h"

namespace scipp {

enum SizeofTag { Underlying, ViewOnly };

SCIPP_DATASET_EXPORT scipp::index size_of(const Variable &view, SizeofTag tag);
SCIPP_DATASET_EXPORT scipp::index size_of(const DataArray &dataarray,
                                          SizeofTag tag,
                                          bool include_aligned_coords = true);
SCIPP_DATASET_EXPORT scipp::index size_of(const Dataset &dataset,
                                          SizeofTag tag);
} // namespace scipp
