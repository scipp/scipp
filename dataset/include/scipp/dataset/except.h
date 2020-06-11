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
#include "scipp/dataset/string.h"
#include "scipp/variable/except.h"

namespace scipp::dataset {

class DataArrayConstView;
class DatasetConstView;
class Dataset;
class DataArray;

} // namespace scipp::dataset

namespace scipp::except {

using DataArrayError = Error<dataset::DataArray>;
using DatasetError = Error<dataset::Dataset>;

using DataArrayMismatchError = MismatchError<dataset::DataArray>;
using DatasetMismatchError = MismatchError<dataset::Dataset>;
using CoordMismatchError = MismatchError<std::pair<Dim, VariableConstView>>;

template <class T>
MismatchError(const dataset::DatasetConstView &, const T &)
    -> MismatchError<dataset::Dataset>;
template <class T>
MismatchError(const dataset::DataArrayConstView &, const T &)
    -> MismatchError<dataset::DataArray>;
template <class T>
MismatchError(const std::pair<Dim, VariableConstView> &, const T &)
    -> MismatchError<std::pair<Dim, VariableConstView>>;
template <class T>
MismatchError(const std::pair<std::string, VariableConstView> &, const T &)
    -> MismatchError<std::pair<std::string, VariableConstView>>;

} // namespace scipp::except

namespace scipp::dataset::expect {

void SCIPP_DATASET_EXPORT coordsAreSuperset(const DataArrayConstView &a,
                                            const DataArrayConstView &b);

} // namespace scipp::dataset::expect
