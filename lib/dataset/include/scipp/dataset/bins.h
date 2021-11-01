// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/generated_bins.h"
#include "scipp/variable/bins.h"

namespace scipp::dataset {

SCIPP_DATASET_EXPORT void copy_slices(const DataArray &src, DataArray dst,
                                      const Dim dim, const Variable &srcIndices,
                                      const Variable &dstIndices);
SCIPP_DATASET_EXPORT void copy_slices(const Dataset &src, Dataset dst,
                                      const Dim dim, const Variable &srcIndices,
                                      const Variable &dstIndices);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray resize_default_init(
    const DataArray &parent, const Dim dim, const scipp::index size);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset resize_default_init(
    const Dataset &parent, const Dim dim, const scipp::index size);

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

[[nodiscard]] SCIPP_DATASET_EXPORT bool is_bins(const DataArray &array);
[[nodiscard]] SCIPP_DATASET_EXPORT bool is_bins(const Dataset &dataset);

} // namespace scipp::dataset

namespace scipp::dataset::buckets {

[[nodiscard]] SCIPP_DATASET_EXPORT Variable concatenate(const Variable &var0,
                                                        const Variable &var1);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray concatenate(const DataArray &var0,
                                                         const DataArray &var1);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable concatenate(const Variable &var,
                                                        const Dim dim);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray concatenate(const DataArray &var,
                                                         const Dim dim);

SCIPP_DATASET_EXPORT void append(Variable &var0, const Variable &var1);
SCIPP_DATASET_EXPORT void append(DataArray &a, const DataArray &b);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable histogram(const Variable &data,
                                                      const Variable &binEdges);

[[nodiscard]] SCIPP_DATASET_EXPORT Variable map(const DataArray &function,
                                                const Variable &x,
                                                Dim hist_dim);

SCIPP_DATASET_EXPORT void scale(DataArray &data, const DataArray &histogram,
                                Dim dim = Dim::Invalid);

} // namespace scipp::dataset::buckets

// These functions are placed in scipp::variable for ADL but cannot be easily
// moved into that compilation module since the implementation requires
// DataArray.
namespace scipp::variable {
[[nodiscard]] SCIPP_DATASET_EXPORT Variable bins_sum(const Variable &data);
[[nodiscard]] SCIPP_DATASET_EXPORT Variable bins_mean(const Variable &data);
} // namespace scipp::variable
