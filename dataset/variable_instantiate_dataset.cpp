// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/string.h"
#include "scipp/core/variable.tcc"
#include "scipp/dataset/dataset.h"

namespace scipp::core {

INSTANTIATE_VARIABLE(scipp::dataset::Dataset)
INSTANTIATE_VARIABLE(scipp::dataset::DataArray)

} // namespace scipp::core

namespace scipp::dataset {
namespace {
// Insert classes from scipp::dataset into formatting registry. The objects
// themselves do nothing, but the constructor call with comma operator does the
// insertion.
auto register_dataset_types(
    (core::formatterRegistry().emplace(
         dtype<Dataset>, std::make_unique<core::Formatter<Dataset>>()),
     core::formatterRegistry().emplace(
         dtype<DataArray>, std::make_unique<core::Formatter<DataArray>>()),
     0));
} // namespace
} // namespace scipp::dataset
