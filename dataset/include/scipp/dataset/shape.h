// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray concatenate(const DataArrayConstView &a,
                                           const DataArrayConstView &b,
                                           const Dim dim);
SCIPP_DATASET_EXPORT Dataset concatenate(const DatasetConstView &a,
                                         const DatasetConstView &b,
                                         const Dim dim);

SCIPP_DATASET_EXPORT DataArray resize(const DataArrayConstView &a,
                                      const Dim dim, const scipp::index size);
SCIPP_DATASET_EXPORT Dataset resize(const DatasetConstView &d, const Dim dim,
                                    const scipp::index size);

} // namespace scipp::dataset
