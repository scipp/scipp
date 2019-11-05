// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_NEUTRON_BEAMLINE_H
#define SCIPP_NEUTRON_BEAMLINE_H

#include "scipp-neutron_export.h"
#include "scipp/core/dataset.h"
#include "scipp/units/unit.h"

namespace scipp::neutron {

SCIPP_NEUTRON_EXPORT core::VariableConstProxy
position(const core::DatasetConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable
source_position(const core::DatasetConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable
sample_position(const core::DatasetConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable l1(const core::DatasetConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable l2(const core::DatasetConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable
scattering_angle(const core::DatasetConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable two_theta(const core::DatasetConstProxy &d);

SCIPP_NEUTRON_EXPORT core::VariableConstProxy
position(const core::DataConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable
source_position(const core::DataConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable
sample_position(const core::DataConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable l1(const core::DataConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable l2(const core::DataConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable
scattering_angle(const core::DataConstProxy &d);
SCIPP_NEUTRON_EXPORT core::Variable two_theta(const core::DataConstProxy &d);

} // namespace scipp::neutron

#endif // SCIPP_NEUTRON_BEAMLINE_H
