// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/units/simple.h"
#include "scipp/units/unit.tcc"

namespace scipp::units {

INSTANTIATE(simple::supported_units);
namespace simple {
SCIPP_UNITS_DEFINE_DIMENSIONS();
}

} // namespace scipp::units
