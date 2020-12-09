// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Owen Arnold
#pragma once

#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

[[nodiscard]] SCIPP_DATASET_EXPORT DataArrayConstView
slice(const DataArrayConstView &data, const Dim dim,
      const VariableConstView value);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArrayConstView
slice(const DataArrayConstView &data, const Dim dim,
      const VariableConstView begin, const VariableConstView end);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArrayView
slice(const DataArrayView &data, const Dim dim, const VariableConstView value);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArrayView
slice(const DataArrayView &data, const Dim dim, const VariableConstView begin,
      const VariableConstView end);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArrayView
slice(DataArray &data, const Dim dim, const VariableConstView value);

[[nodiscard]] SCIPP_DATASET_EXPORT DataArrayView
slice(DataArray &data, const Dim dim, const VariableConstView begin,
      const VariableConstView end);

[[nodiscard]] SCIPP_DATASET_EXPORT DatasetConstView
slice(const DatasetConstView &ds, const Dim dim, const VariableConstView value);

[[nodiscard]] SCIPP_DATASET_EXPORT DatasetView
slice(const DatasetView &ds, const Dim dim, const VariableConstView value);

[[nodiscard]] SCIPP_DATASET_EXPORT DatasetView
slice(Dataset &ds, const Dim dim, const VariableConstView value);

[[nodiscard]] SCIPP_DATASET_EXPORT DatasetConstView
slice(const DatasetConstView &ds, const Dim dim, const VariableConstView begin,
      const VariableConstView end);

[[nodiscard]] SCIPP_DATASET_EXPORT DatasetView
slice(const DatasetView &ds, const Dim dim, const VariableConstView begin,
      const VariableConstView end);

[[nodiscard]] SCIPP_DATASET_EXPORT DatasetView
slice(Dataset &ds, const Dim dim, const VariableConstView begin,
      const VariableConstView end);

} // namespace scipp::dataset

namespace scipp::variable {
[[nodiscard]] SCIPP_DATASET_EXPORT std::tuple<Dim, scipp::index>
get_slice_params(const Dimensions &dims, const VariableConstView &coord,
                 const VariableConstView value);
[[nodiscard]] SCIPP_DATASET_EXPORT std::tuple<Dim, scipp::index, scipp::index>
get_slice_params(const Dimensions &dims, const VariableConstView &coord,
                 const VariableConstView begin, const VariableConstView end);
[[nodiscard]] SCIPP_DATASET_EXPORT VariableConstView
select(const VariableConstView &var, const VariableConstView &coord,
       const VariableConstView &value);
[[nodiscard]] SCIPP_DATASET_EXPORT VariableConstView
select(const VariableConstView &var, const VariableConstView &coord,
       const VariableConstView &begin, const VariableConstView &end);
} // namespace scipp::variable
