// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_NEUTRON_BEAMLINE_H
#define SCIPP_NEUTRON_BEAMLINE_H

#include <vector>

#include "scipp-neutron_export.h"
#include "scipp/units/unit.h"

namespace scipp::core {
class Dataset;
class Variable;
} // namespace scipp::core

namespace scipp::neutron {

SCIPP_NEUTRON_EXPORT core::Variable source_position(const core::Dataset &d);
SCIPP_NEUTRON_EXPORT core::Variable sample_position(const core::Dataset &d);
SCIPP_NEUTRON_EXPORT core::Variable l1(const core::Dataset &d);
SCIPP_NEUTRON_EXPORT core::Variable l2(const core::Dataset &d);
SCIPP_NEUTRON_EXPORT core::Variable scattering_angle(const core::Dataset &d);
SCIPP_NEUTRON_EXPORT core::Variable two_theta(const core::Dataset &d);

} // namespace scipp::neutron

#endif // SCIPP_NEUTRON_BEAMLINE_H
