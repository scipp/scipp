// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/string.h"
#include "scipp/variable/bucket_variable.tcc"
#include "scipp/variable/string.h"

namespace scipp::variable {

INSTANTIATE_BUCKET_VARIABLE(bucket_Dataset, bucket<Dataset>)
INSTANTIATE_BUCKET_VARIABLE(bucket_DataArray, bucket<DataArray>)

} // namespace scipp::variable

namespace scipp::dataset {
namespace {
auto register_dataset_types(
    (variable::formatterRegistry().emplace(
         dtype<bucket<Dataset>>,
         std::make_unique<variable::Formatter<bucket<Dataset>>>()),
     variable::formatterRegistry().emplace(
         dtype<bucket<DataArray>>,
         std::make_unique<variable::Formatter<bucket<DataArray>>>()),
     0));
} // namespace
} // namespace scipp::dataset
