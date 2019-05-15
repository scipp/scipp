// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
/// @author Neil Vaytet
#include "scipp/units/unit.tcc"

namespace scipp::units {

#define INSTANTIATE(Units)                                                     \
  template class Unit_impl<Units>;                                             \
  template Unit_impl<Units> operator+(const Unit_impl<Units> &,                \
                                      const Unit_impl<Units> &);               \
  template Unit_impl<Units> operator-(const Unit_impl<Units> &,                \
                                      const Unit_impl<Units> &);               \
  template Unit_impl<Units> operator*(const Unit_impl<Units> &,                \
                                      const Unit_impl<Units> &);               \
  template Unit_impl<Units> operator/(const Unit_impl<Units> &,                \
                                      const Unit_impl<Units> &);               \
  template Unit_impl<Units> sqrt(const Unit_impl<Units> &a);                   \
  template bool containsCounts(const Unit_impl<Units> &unit);

INSTANTIATE(neutron::supported_units);

} // namespace scipp::units
