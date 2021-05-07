// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/string.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

INSTANTIATE_VARIABLE(Dataset, scipp::dataset::Dataset)
INSTANTIATE_VARIABLE(DataArray, scipp::dataset::DataArray)

} // namespace scipp::variable

namespace scipp::dataset {
REGISTER_FORMATTER(DataArray, DataArray)
REGISTER_FORMATTER(Dataset, Dataset)
} // namespace scipp::dataset
