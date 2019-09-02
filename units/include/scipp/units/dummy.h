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

class SCIPP_UNITS_EXPORT Unit : public Unit_impl<Unit> {
public:
  using Unit_impl<Unit>::Unit_impl;
};

SCIPP_UNITS_DECLARE_DIMENSIONS(X, Y, Z)

} // namespace dummy

template <> struct supported_units<dummy::Unit> {
  using type = decltype(detail::make_unit(
      std::make_tuple(m),
      std::make_tuple(dimensionless, rad, deg, dimensionless / m, s,
                      dimensionless / s, m / s)));
};
template <> struct counts_unit<dummy::Unit> {
  using type = decltype(dimensionless);
};

} // namespace scipp::units

#endif // SCIPP_UNITS_DUMMY_H
