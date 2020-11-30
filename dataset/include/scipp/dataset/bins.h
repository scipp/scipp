// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"
#include "scipp/variable/bins.h"

namespace scipp::dataset {

SCIPP_DATASET_EXPORT void copy_slices(const DataArrayConstView &src,
                                      const DataArrayView &dst, const Dim dim,
                                      const VariableConstView &srcIndices,
                                      const VariableConstView &dstIndices);
SCIPP_DATASET_EXPORT void copy_slices(const DatasetConstView &src,
                                      const DatasetView &dst, const Dim dim,
                                      const VariableConstView &srcIndices,
                                      const VariableConstView &dstIndices);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray resize_default_init(
    const DataArrayConstView &parent, const Dim dim, const scipp::index size);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset resize_default_init(
    const DatasetConstView &parent, const Dim dim, const scipp::index size);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable make_bins(Variable indices,
                                                      const Dim dim,
                                                      DataArray buffer);
[[nodiscard]] SCIPP_DATASET_EXPORT Variable make_bins(Variable indices,
                                                      const Dim dim,
                                                      Dataset buffer);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable make_non_owning_bins(
    const VariableView &indices, const Dim dim, const DataArrayView &buffer);
[[nodiscard]] SCIPP_DATASET_EXPORT Variable
make_non_owning_bins(const VariableConstView &indices, const Dim dim,
                     const DataArrayConstView &buffer);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
bucket_sizes(const VariableConstView &var);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
bucket_sizes(const DataArrayConstView &array);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset
bucket_sizes(const DatasetConstView &dataset);

[[nodiscard]] SCIPP_DATASET_EXPORT bool
is_buckets(const DataArrayConstView &array);
[[nodiscard]] SCIPP_DATASET_EXPORT bool
is_buckets(const DatasetConstView &dataset);

} // namespace scipp::dataset

namespace scipp::dataset::buckets {

SCIPP_DATASET_EXPORT void reserve(const VariableView &var,
                                  const VariableConstView &shape);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
concatenate(const VariableConstView &var0, const VariableConstView &var1);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
concatenate(const DataArrayConstView &var0, const DataArrayConstView &var1);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
concatenate(const VariableConstView &var, const Dim dim);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
concatenate(const DataArrayConstView &var, const Dim dim);

SCIPP_DATASET_EXPORT void append(const VariableView &var0,
                                 const VariableConstView &var1);
SCIPP_DATASET_EXPORT void append(const DataArrayView &a,
                                 const DataArrayConstView &b);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
histogram(const VariableConstView &data, const VariableConstView &binEdges);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
map(const DataArrayConstView &function, const VariableConstView &x,
    Dim hist_dim);

void scale(const DataArrayView &data, const DataArrayConstView &histogram,
           Dim dim = Dim::Invalid);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable sum(const VariableConstView &data);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
sum(const DataArrayConstView &data);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset sum(const DatasetConstView &data);

} // namespace scipp::dataset::buckets
