// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_NEUTRON_CONVERT_H
#define SCIPP_NEUTRON_CONVERT_H

#include <vector>

#include "scipp-neutron_export.h"
#include "scipp/core/dataset.h"
#include "scipp/units/unit.h"

namespace scipp::neutron {

SCIPP_NEUTRON_EXPORT core::DataArray convert(core::DataArray d, const Dim from,
                                             const Dim to);
SCIPP_NEUTRON_EXPORT core::Dataset convert(core::Dataset d, const Dim from,
                                           const Dim to);

} // namespace scipp::neutron

#endif // SCIPP_NEUTRON_CONVERT_H
