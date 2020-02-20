// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_NEUTRON_BEAMLINE_H
#define SCIPP_NEUTRON_BEAMLINE_H

#include "scipp-neutron_export.h"
#include "scipp/core/dataset.h"
#include "scipp/units/unit.h"

namespace scipp::neutron {

SCIPP_NEUTRON_EXPORT core::VariableConstView
position(const core::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableConstView
source_position(const core::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableConstView
sample_position(const core::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableView position(const core::DatasetView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
source_position(const core::DatasetView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
sample_position(const core::DatasetView &d);
SCIPP_NEUTRON_EXPORT core::Variable
flight_path_length(const core::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable l1(const core::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable l2(const core::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable
scattering_angle(const core::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable two_theta(const core::DatasetConstView &d);

SCIPP_NEUTRON_EXPORT core::VariableConstView
position(const core::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableConstView
source_position(const core::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableConstView
sample_position(const core::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableView position(const core::DataArrayView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
source_position(const core::DataArrayView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
sample_position(const core::DataArrayView &d);
SCIPP_NEUTRON_EXPORT core::Variable
flight_path_length(const core::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable l1(const core::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable l2(const core::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable
scattering_angle(const core::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable
two_theta(const core::DataArrayConstView &d);

} // namespace scipp::neutron

#endif // SCIPP_NEUTRON_BEAMLINE_H
