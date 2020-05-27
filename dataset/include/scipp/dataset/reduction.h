// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray flatten(const DataArrayConstView &a,
                                       const Dim dim);
SCIPP_DATASET_EXPORT Dataset flatten(const DatasetConstView &d, const Dim dim);

SCIPP_DATASET_EXPORT DataArray sum(const DataArrayConstView &a, const Dim dim);
SCIPP_DATASET_EXPORT Dataset sum(const DatasetConstView &d, const Dim dim);

SCIPP_DATASET_EXPORT DataArray mean(const DataArrayConstView &a, const Dim dim);
SCIPP_DATASET_EXPORT Dataset mean(const DatasetConstView &d, const Dim dim);

} // namespace scipp::dataset
