// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_NEUTRON_BEAMLINE_H
#define SCIPP_NEUTRON_BEAMLINE_H

#include "scipp-neutron_export.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/unit.h"

namespace scipp::neutron {

SCIPP_NEUTRON_EXPORT core::VariableConstView
position(const dataset::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableConstView
source_position(const dataset::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableConstView
sample_position(const dataset::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableView position(const dataset::DatasetView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
source_position(const dataset::DatasetView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
sample_position(const dataset::DatasetView &d);
SCIPP_NEUTRON_EXPORT core::Variable
flight_path_length(const dataset::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable l1(const dataset::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable l2(const dataset::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable
scattering_angle(const dataset::DatasetConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable
two_theta(const dataset::DatasetConstView &d);

SCIPP_NEUTRON_EXPORT core::VariableConstView
position(const dataset::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableConstView
source_position(const dataset::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableConstView
sample_position(const dataset::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
position(const dataset::DataArrayView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
source_position(const dataset::DataArrayView &d);
SCIPP_NEUTRON_EXPORT core::VariableView
sample_position(const dataset::DataArrayView &d);
SCIPP_NEUTRON_EXPORT core::Variable
flight_path_length(const dataset::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable l1(const dataset::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable l2(const dataset::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable
scattering_angle(const dataset::DataArrayConstView &d);
SCIPP_NEUTRON_EXPORT core::Variable
two_theta(const dataset::DataArrayConstView &d);

} // namespace scipp::neutron

#endif // SCIPP_NEUTRON_BEAMLINE_H
