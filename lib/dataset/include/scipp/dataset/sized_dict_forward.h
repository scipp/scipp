// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-dataset_export.h"
#include "scipp/units/dim.h"

namespace scipp::variable {
class Variable;
}

namespace scipp::dataset {

template <class Key, class Value, class Impl = void> class SizedDict;
template <class Key, class Value> class AlignedDict;
template <class Value> struct AlignedValue;

/// Dict of coordinates of DataArray and Dataset.
using Coords = AlignedDict<Dim, variable::Variable>;
/// Dict of masks of DataArray and Dataset.
using Masks = SizedDict<std::string, variable::Variable>;
/// Dict of attributes of DataArray and Dataset.
using Attrs = AlignedDict<Dim, variable::Variable>;
/// Underlying dict of coordinates.
/// This alias is needed because Coords::holder_type cannot be accessed when
/// only the forward declaration is visible.
using CoordsHolder = SizedDict<Dim, AlignedValue<variable::Variable>>;

[[nodiscard]] SCIPP_DATASET_EXPORT Coords copy(const Coords &coords);
[[nodiscard]] SCIPP_DATASET_EXPORT Masks copy(const Masks &masks);

} // namespace scipp::dataset
