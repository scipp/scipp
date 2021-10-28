// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <set>
#include <tuple>

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray histogram(const DataArray &events,
                                         const Variable &binEdges);
SCIPP_DATASET_EXPORT Dataset histogram(const Dataset &dataset,
                                       const Variable &bins);

SCIPP_DATASET_EXPORT std::set<Dim> edge_dimensions(const DataArray &a);
SCIPP_DATASET_EXPORT Dim edge_dimension(const DataArray &a);
SCIPP_DATASET_EXPORT bool is_histogram(const DataArray &a, const Dim dim);
SCIPP_DATASET_EXPORT bool is_histogram(const Dataset &a, const Dim dim);

} // namespace scipp::dataset
