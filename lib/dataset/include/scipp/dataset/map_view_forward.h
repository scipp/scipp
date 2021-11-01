// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-dataset_export.h"
#include "scipp/units/dim.h"

namespace scipp::variable {
class Variable;
}

namespace scipp::dataset {

template <class Key, class Value> class Dict;

/// Dict of coordinates of DataArray and Dataset.
using Coords = Dict<Dim, variable::Variable>;
/// Dict of masks of DataArray and Dataset.
using Masks = Dict<std::string, variable::Variable>;
/// Dict of attributes of DataArray and Dataset.
using Attrs = Dict<Dim, variable::Variable>;

[[nodiscard]] SCIPP_DATASET_EXPORT Coords copy(const Coords &coords);
[[nodiscard]] SCIPP_DATASET_EXPORT Masks copy(const Masks &masks);

} // namespace scipp::dataset
