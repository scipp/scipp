// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/flags.h"
#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray concatenate(const DataArray &a,
                                                         const DataArray &b,
                                                         const Dim dim);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset concatenate(const Dataset &a,
                                                       const Dataset &b,
                                                       const Dim dim);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
resize(const DataArray &a, const Dim dim, const scipp::index size,
       const FillValue fill = FillValue::Default);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset
resize(const Dataset &d, const Dim dim, const scipp::index size,
       const FillValue fill = FillValue::Default);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray resize(const DataArray &a,
                                                    const Dim dim,
                                                    const DataArray &shape);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset resize(const Dataset &d,
                                                  const Dim dim,
                                                  const Dataset &shape);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray fold(const DataArray &a,
                                                  const Dim from_dim,
                                                  const Dimensions &to_dims);
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
flatten(const DataArray &a, const scipp::span<const Dim> &from_labels,
        const Dim to_dim);

} // namespace scipp::dataset
