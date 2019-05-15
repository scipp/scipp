// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file Simple and small system of units for testing purposes.
/// @author Simon Heybrock
#ifndef SCIPP_UNITS_SIMPLE_H
#define SCIPP_UNITS_SIMPLE_H

#include "scipp/units/unit_impl.h"

namespace scipp::units {
#if SCIPP_UNITS == simple
inline
#endif
    namespace simple {
using supported_units = decltype(detail::make_unit(
    std::make_tuple(m), std::make_tuple(dimensionless, dimensionless / m, s,
                                        dimensionless / s, m / s)));

using Unit = Unit_impl<supported_units>;
} // namespace simple
} // namespace scipp::units

#endif // SCIPP_UNITS_SIMPLE_H
