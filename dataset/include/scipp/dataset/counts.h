// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <vector>

#include "scipp-dataset_export.h"
#include "scipp/dataset/dataset.h"

namespace scipp::dataset::counts {

SCIPP_DATASET_EXPORT void toDensity(const DataArrayView data,
                                    const std::vector<Variable> &binWidths);
SCIPP_DATASET_EXPORT Dataset toDensity(Dataset d, const Dim dim);
SCIPP_DATASET_EXPORT Dataset toDensity(Dataset d, const std::vector<Dim> &dims);
SCIPP_DATASET_EXPORT DataArray toDensity(DataArray a, const Dim dim);
SCIPP_DATASET_EXPORT DataArray toDensity(DataArray a,
                                         const std::vector<Dim> &dims);
SCIPP_DATASET_EXPORT void fromDensity(const DataArrayView data,
                                      const std::vector<Variable> &binWidths);
SCIPP_DATASET_EXPORT Dataset fromDensity(Dataset d, const Dim dim);
SCIPP_DATASET_EXPORT Dataset fromDensity(Dataset d,
                                         const std::vector<Dim> &dims);
SCIPP_DATASET_EXPORT DataArray fromDensity(DataArray a, const Dim dim);
SCIPP_DATASET_EXPORT DataArray fromDensity(DataArray a,
                                           const std::vector<Dim> &dims);

} // namespace scipp::dataset::counts
