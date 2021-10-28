// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#pragma once

#include "scipp/core/flags.h"

#include "scipp-dataset_export.h"
#include "scipp/dataset/data_array.h"

namespace scipp::dataset {

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
to_unit(const DataArray &array, const units::Unit &unit,
        CopyPolicy copy = CopyPolicy::Always);

} // namespace scipp::dataset
