// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
