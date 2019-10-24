// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Mads Bertelsen
#ifndef SCIPP_NEUTRON_CONVERT_WITH_CALIBRATION_H
#define SCIPP_NEUTRON_CONVERT_WITH_CALIBRATION_H

#include "scipp-neutron_export.h"
#include "scipp/core/dataset.h"

namespace scipp::neutron::diffraction {

SCIPP_NEUTRON_EXPORT core::Dataset convert_with_calibration(core::Dataset d, const core::Dataset &cal);

} // namespace scipp::neutron::diffraction

#endif // SCIPP_NEUTRON_CONVERT_WITH_CALIBRATION_H
