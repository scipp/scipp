// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <optional>

#include "scipp/core/flags.h"
#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
concat(const std::span<const DataArray> das, const Dim dim);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset
concat(const std::span<const Dataset> dss, const Dim dim);

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
[[nodiscard]] SCIPP_DATASET_EXPORT DataArray flatten(
    const DataArray &a, const std::optional<std::span<const Dim>> &from_labels,
    const Dim to_dim);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
transpose(const DataArray &a, std::span<const Dim> dims = {});
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset
transpose(const Dataset &d, std::span<const Dim> dims = {});

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
squeeze(const DataArray &a, std::optional<std::span<const Dim>> dims);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset
squeeze(const Dataset &a, std::optional<std::span<const Dim>> dims);
} // namespace scipp::dataset
