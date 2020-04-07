// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/variable.tcc"
#include "scipp/dataset/dataset.h"

namespace scipp::dataset {

INSTANTIATE_VARIABLE(Dataset)
INSTANTIATE_VARIABLE(sparse_container<Dataset>)
INSTANTIATE_VARIABLE(DataArray)
INSTANTIATE_VARIABLE(sparse_container<DataArray>)

} // namespace scipp::dataset
