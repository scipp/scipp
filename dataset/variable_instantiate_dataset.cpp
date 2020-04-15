// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset.h"
#include "scipp/variable/string.h"
#include "scipp/variable/variable.tcc"

namespace scipp::variable {

INSTANTIATE_VARIABLE(Dataset, scipp::dataset::Dataset)
INSTANTIATE_VARIABLE(DataArray, scipp::dataset::DataArray)

} // namespace scipp::variable

namespace scipp::dataset {
namespace {
// Insert classes from scipp::dataset into formatting registry. The objects
// themselves do nothing, but the constructor call with comma operator does the
// insertion.
auto register_dataset_types(
    (variable::formatterRegistry().emplace(
         dtype<Dataset>, std::make_unique<variable::Formatter<Dataset>>()),
     variable::formatterRegistry().emplace(
         dtype<DataArray>, std::make_unique<variable::Formatter<DataArray>>()),
     0));
} // namespace
} // namespace scipp::dataset
