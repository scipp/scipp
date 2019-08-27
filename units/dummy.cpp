// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/units/dummy.h"
#include "scipp/units/unit.tcc"

namespace scipp::units {

INSTANTIATE(dummy::Unit)
SCIPP_UNITS_DEFINE_DIMENSIONS(dummy)

} // namespace scipp::units
