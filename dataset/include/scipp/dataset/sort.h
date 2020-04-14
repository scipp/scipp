// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <vector>

#include <scipp/dataset/dataset.h>
#include <scipp/variable/variable.h>

namespace scipp::dataset {

SCIPP_DATASET_EXPORT Variable sort(const VariableConstView &var,
                                   const VariableConstView &key);
SCIPP_DATASET_EXPORT DataArray sort(const DataArrayConstView &array,
                                    const VariableConstView &key);
SCIPP_DATASET_EXPORT DataArray sort(const DataArrayConstView &array,
                                    const Dim &key);
SCIPP_DATASET_EXPORT Dataset sort(const DatasetConstView &dataset,
                                  const VariableConstView &key);
SCIPP_DATASET_EXPORT Dataset sort(const DatasetConstView &dataset,
                                  const Dim &key);

} // namespace scipp::dataset
