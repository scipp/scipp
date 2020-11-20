// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <scipp/dataset/dataset.h>

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray sortby(const DataArrayConstView &data,
                                      const Dim dim);
SCIPP_DATASET_EXPORT DataArray
bin(const DataArrayConstView &array,
    const std::vector<VariableConstView> &edges,
    const std::vector<VariableConstView> &groups = {});

} // namespace scipp::dataset
