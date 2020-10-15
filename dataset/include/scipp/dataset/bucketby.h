// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <scipp/dataset/dataset.h>

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray sortby(const DataArrayConstView &data,
                                      const Dim dim);
SCIPP_DATASET_EXPORT Variable bucketby(const DataArrayConstView &data,
                                       const Dim dim,
                                       const VariableConstView &bins);
SCIPP_DATASET_EXPORT Variable bucketby(const DataArrayConstView &data,
                                       const Dim dim0,
                                       const VariableConstView &bins0,
                                       const Dim dim1,
                                       const VariableConstView &bins1);

} // namespace scipp::dataset
