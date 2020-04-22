// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Mads Bertelsen
#pragma once

#include "scipp-neutron_export.h"
#include "scipp/dataset/dataset.h"

namespace scipp::neutron::diffraction {

SCIPP_NEUTRON_EXPORT dataset::DataArray
convert_with_calibration(dataset::DataArray d, dataset::Dataset cal);
SCIPP_NEUTRON_EXPORT dataset::Dataset
convert_with_calibration(dataset::Dataset d, dataset::Dataset cal);

} // namespace scipp::neutron::diffraction
