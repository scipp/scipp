// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <vector>

#include "scipp-dataset_export.h"
#include "scipp/dataset/dataset.h"

namespace scipp::dataset::counts {

SCIPP_DATASET_EXPORT void toDensity(DataArray &data,
                                    const std::vector<Variable> &binWidths);
SCIPP_DATASET_EXPORT Dataset toDensity(const Dataset &d, const Dim dim);
SCIPP_DATASET_EXPORT Dataset toDensity(const Dataset &d,
                                       const std::vector<Dim> &dims);
SCIPP_DATASET_EXPORT DataArray toDensity(const DataArray &a, const Dim dim);
SCIPP_DATASET_EXPORT DataArray toDensity(const DataArray &a,
                                         const std::vector<Dim> &dims);
SCIPP_DATASET_EXPORT void fromDensity(DataArray &data,
                                      const std::vector<Variable> &binWidths);
SCIPP_DATASET_EXPORT Dataset fromDensity(const Dataset &d, const Dim dim);
SCIPP_DATASET_EXPORT Dataset fromDensity(const Dataset &d,
                                         const std::vector<Dim> &dims);
SCIPP_DATASET_EXPORT DataArray fromDensity(const DataArray &a, const Dim dim);
SCIPP_DATASET_EXPORT DataArray fromDensity(const DataArray &a,
                                           const std::vector<Dim> &dims);

} // namespace scipp::dataset::counts
