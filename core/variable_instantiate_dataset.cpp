// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/core/variable.tcc"

namespace scipp::core {

INSTANTIATE_VARIABLE(Dataset)
INSTANTIATE_VARIABLE(sparse_container<Dataset>)
INSTANTIATE_VARIABLE(DataArray)
INSTANTIATE_VARIABLE(sparse_container<DataArray>)

} // namespace scipp::core
