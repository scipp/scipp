// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"
#include "scipp/variable/bins.h"

namespace scipp::dataset {

SCIPP_DATASET_EXPORT void copy_slices(const DataArrayConstView &src,
                                      DataArray dst, const Dim dim,
                                      const Variable &srcIndices,
                                      const Variable &dstIndices);
SCIPP_DATASET_EXPORT void copy_slices(const DatasetConstView &src, Dataset dst,
                                      const Dim dim, const Variable &srcIndices,
                                      const Variable &dstIndices);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray resize_default_init(
    const DataArrayConstView &parent, const Dim dim, const scipp::index size);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset resize_default_init(
    const DatasetConstView &parent, const Dim dim, const scipp::index size);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable make_bins(Variable indices,
                                                      const Dim dim,
                                                      DataArray buffer);
[[nodiscard]] SCIPP_DATASET_EXPORT Variable
make_bins_no_validate(Variable indices, const Dim dim, DataArray buffer);
[[nodiscard]] SCIPP_DATASET_EXPORT Variable make_bins(Variable indices,
                                                      const Dim dim,
                                                      Dataset buffer);
[[nodiscard]] SCIPP_DATASET_EXPORT Variable
make_bins_no_validate(Variable indices, const Dim dim, Dataset buffer);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable bucket_sizes(const Variable &var);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
bucket_sizes(const DataArrayConstView &array);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset
bucket_sizes(const DatasetConstView &dataset);

[[nodiscard]] SCIPP_DATASET_EXPORT bool
is_bins(const DataArrayConstView &array);
[[nodiscard]] SCIPP_DATASET_EXPORT bool
is_bins(const DatasetConstView &dataset);

} // namespace scipp::dataset

namespace scipp::dataset::buckets {

SCIPP_DATASET_EXPORT void reserve(Variable &var, const Variable &shape);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable concatenate(const Variable &var0,
                                                        const Variable &var1);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
concatenate(const DataArrayConstView &var0, const DataArrayConstView &var1);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable concatenate(const Variable &var,
                                                        const Dim dim);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
concatenate(const DataArrayConstView &var, const Dim dim);

SCIPP_DATASET_EXPORT void append(Variable &var0, const Variable &var1);
SCIPP_DATASET_EXPORT void append(DataArray &a, const DataArrayConstView &b);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable histogram(const Variable &data,
                                                      const Variable &binEdges);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable
map(const DataArrayConstView &function, const Variable &x, Dim hist_dim);

SCIPP_DATASET_EXPORT void scale(DataArray &data,
                                const DataArrayConstView &histogram,
                                Dim dim = Dim::Invalid);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable sum(const Variable &data);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
sum(const DataArrayConstView &data);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset sum(const DatasetConstView &data);

} // namespace scipp::dataset::buckets
