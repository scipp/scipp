// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <set>
#include <tuple>

#include "scipp/dataset/except.h"

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray histogram(const DataArrayConstView &events,
                                         const VariableConstView &binEdges);
SCIPP_DATASET_EXPORT Dataset histogram(const DatasetConstView &dataset,
                                       const VariableConstView &bins);
SCIPP_DATASET_EXPORT DataArray histogram(const DataArrayConstView &realigned);
SCIPP_DATASET_EXPORT Dataset histogram(const DatasetConstView &realigned);

SCIPP_DATASET_EXPORT std::set<Dim> edge_dimensions(const DataArrayConstView &a);
SCIPP_DATASET_EXPORT Dim edge_dimension(const DataArrayConstView &a);
SCIPP_DATASET_EXPORT bool is_histogram(const DataArrayConstView &a,
                                       const Dim dim);

} // namespace scipp::dataset
