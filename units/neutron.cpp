// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/units/neutron.h"
#include "scipp/units/unit.tcc"

namespace scipp::units {
INSTANTIATE(neutron::supported_units, neutron::counts_unit);
SCIPP_UNITS_DEFINE_DIMENSIONS(neutron);

namespace neutron {
bool containsCounts(const Unit &unit) {
  if ((unit == counts) || unit == counts / us || unit == counts / meV)
    return true;
  return false;
}
} // namespace neutron
} // namespace scipp::units
