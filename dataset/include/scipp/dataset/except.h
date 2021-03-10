// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

struct SCIPP_DATASET_EXPORT DataArrayError : public Error<dataset::DataArray> {
  explicit DataArrayError(const std::string &msg);
};

SCIPP_DATASET_EXPORT DataArrayError
mismatch_error(const dataset::DataArrayConstView &expected,
               const dataset::DataArrayConstView &actual);

struct SCIPP_DATASET_EXPORT DatasetError : public Error<dataset::Dataset> {
  explicit DatasetError(const std::string &msg);
};

SCIPP_DATASET_EXPORT DatasetError
mismatch_error(const dataset::DatasetConstView &expected,
               const dataset::DatasetConstView &actual);

using CoordMismatchError = MismatchError<std::pair<Dim, VariableConstView>>;

template <class T>
MismatchError(const std::pair<Dim, VariableConstView> &, const T &)
    -> MismatchError<std::pair<Dim, VariableConstView>>;
template <class T>
MismatchError(const std::pair<std::string, VariableConstView> &, const T &)
    -> MismatchError<std::pair<std::string, VariableConstView>>;

template struct SCIPP_DATASET_EXPORT
    MismatchError<std::pair<Dim, VariableConstView>>;

} // namespace scipp::except

namespace scipp::dataset::expect {

SCIPP_DATASET_EXPORT void coordsAreSuperset(const DataArrayConstView &a,
                                            const DataArrayConstView &b);

SCIPP_DATASET_EXPORT void isKey(const VariableConstView &key);

} // namespace scipp::dataset::expect
