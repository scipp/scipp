// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/units/dim.h"

namespace scipp::variable {
class Variable;
}

namespace scipp::dataset {

template <class Key, class Value> class Dict;

/// Dict of coordinates or attributes of DataArray and Dataset.
using CoordsDict = Dict<Dim, variable::Variable>;
/// Dict of coordinates or attributes of DataArray and Dataset.
using MasksDict = Dict<std::string, variable::Variable>;

} // namespace scipp::dataset
