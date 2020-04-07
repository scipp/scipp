// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "scipp-dataset_export.h"
#include "scipp/core/except.h"

namespace scipp::dataset {

class DataArrayConstView;
class DatasetConstView;
class Dataset;
class DataArray;

} // namespace scipp::dataset

namespace scipp::except {

using DataArrayError = Error<core::DataArray>;
using DatasetError = Error<core::Dataset>;

using DataArrayMismatchError = MismatchError<core::DataArray>;
using DatasetMismatchError = MismatchError<core::Dataset>;

template <class T>
MismatchError(const core::DatasetConstView &, const T &)
    ->MismatchError<core::Dataset>;
template <class T>
MismatchError(const core::DataArrayConstView &, const T &)
    ->MismatchError<core::DataArray>;

} // namespace scipp::except

namespace scipp::dataset::expect {

void SCIPP_DATASET_EXPORT coordsAreSuperset(const DataArrayConstView &a,
                                            const DataArrayConstView &b);

} // namespace scipp::dataset::expect
