// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/variable.tcc"
#include "scipp/dataset/dataset.h"

namespace scipp::core {

INSTANTIATE_VARIABLE(scipp::dataset::Dataset)
INSTANTIATE_VARIABLE(scipp::dataset::DataArray)

} // namespace scipp::core
