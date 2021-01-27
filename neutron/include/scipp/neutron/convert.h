// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-neutron_export.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/unit.h"

namespace scipp::neutron {

[[nodiscard]] SCIPP_NEUTRON_EXPORT dataset::DataArray
convert(dataset::DataArray d, const Dim from, const Dim to);
[[nodiscard]] SCIPP_NEUTRON_EXPORT dataset::DataArray
convert(const dataset::DataArrayConstView &d, const Dim from, const Dim to);
[[nodiscard]] SCIPP_NEUTRON_EXPORT dataset::Dataset
convert(dataset::Dataset d, const Dim from, const Dim to);
[[nodiscard]] SCIPP_NEUTRON_EXPORT dataset::Dataset
convert(const dataset::DatasetConstView &d, const Dim from, const Dim to);

} // namespace scipp::neutron
