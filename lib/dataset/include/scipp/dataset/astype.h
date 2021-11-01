// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "scipp/core/flags.h"

#include "scipp-dataset_export.h"
#include "scipp/dataset/data_array.h"

namespace scipp::dataset {
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray astype(
    const DataArray &array, DType type, CopyPolicy copy = CopyPolicy::Always);
}
