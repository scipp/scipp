// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file Simple and small system of units for testing purposes.
/// @author Simon Heybrock
#ifndef SCIPP_UNITS_DUMMY_H
#define SCIPP_UNITS_DUMMY_H

#include "scipp/units/dimension.h"
#include "scipp/units/unit_impl.h"

namespace scipp::units {
#ifdef SCIPP_UNITS_DUMMY
inline
#endif
    namespace dummy {

using supported_units = decltype(detail::make_unit(
    std::make_tuple(m), std::make_tuple(dimensionless, dimensionless / m, s,
                                        dimensionless / s, m / s)));

using counts_unit = decltype(dimensionless);
using Unit = Unit_impl<supported_units, counts_unit>;

SCIPP_UNITS_DECLARE_DIMENSIONS(X, Y, Z);

} // namespace dummy
} // namespace scipp::units

#endif // SCIPP_UNITS_DUMMY_H
