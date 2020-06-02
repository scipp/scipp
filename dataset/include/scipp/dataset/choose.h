// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <scipp/dataset/dataset.h>

namespace scipp::dataset {

SCIPP_DATASET_EXPORT DataArray choose(const VariableConstView &key,
                                      const DataArrayConstView &choices,
                                      const Dim dim);

} // namespace scipp::dataset
