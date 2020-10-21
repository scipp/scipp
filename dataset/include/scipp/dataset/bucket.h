// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset::buckets {

[[nodiscard]] SCIPP_DATASET_EXPORT std::tuple<Variable, scipp::index>
sizes_to_begin(const VariableConstView &sizes);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
concatenate(const VariableConstView &var0, const VariableConstView &var1);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
concatenate(const DataArrayConstView &var0, const DataArrayConstView &var1);

SCIPP_DATASET_EXPORT void append(const VariableView &var0,
                                 const VariableConstView &var1);
SCIPP_DATASET_EXPORT void append(const DataArrayView &a,
                                 const DataArrayConstView &b);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
histogram(const VariableConstView &data, const VariableConstView &binEdges);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
map(const DataArrayConstView &function, const VariableConstView &x,
    Dim hist_dim);

void scale(const DataArrayView &data, const DataArrayConstView &histogram);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable sum(const VariableConstView &data);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
sum(const DataArrayConstView &data);

[[nodiscard]] Variable from_constituents(Variable &&indices, const Dim dim,
                                         Variable &&buffer);
[[nodiscard]] Variable from_constituents(Variable &&indices, const Dim dim,
                                         DataArray &&buffer);

} // namespace scipp::dataset::buckets
