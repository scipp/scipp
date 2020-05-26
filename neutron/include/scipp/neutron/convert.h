// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-neutron_export.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/unit.h"

namespace scipp::neutron {

enum class ConvertRealign { None, Linear };

SCIPP_NEUTRON_EXPORT dataset::DataArray
convert(dataset::DataArray d, const Dim from, const Dim to,
        const ConvertRealign realign = ConvertRealign::None);
SCIPP_NEUTRON_EXPORT dataset::DataArray
convert(const dataset::DataArrayConstView &d, const Dim from, const Dim to,
        const ConvertRealign realign = ConvertRealign::None);
SCIPP_NEUTRON_EXPORT dataset::Dataset
convert(dataset::Dataset d, const Dim from, const Dim to,
        const ConvertRealign realign = ConvertRealign::None);
SCIPP_NEUTRON_EXPORT dataset::Dataset
convert(const dataset::DatasetConstView &d, const Dim from, const Dim to,
        const ConvertRealign realign = ConvertRealign::None);

} // namespace scipp::neutron
