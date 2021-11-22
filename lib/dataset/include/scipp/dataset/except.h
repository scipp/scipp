// SPDX-License-Identifier: BSD-3-Clause
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

class Dataset;
class DataArray;

} // namespace scipp::dataset

namespace scipp::except {

struct SCIPP_DATASET_EXPORT DataArrayError : public Error<dataset::DataArray> {
  explicit DataArrayError(const std::string &msg);
};

template <>
[[noreturn]] SCIPP_DATASET_EXPORT void
throw_mismatch_error(const dataset::DataArray &expected,
                     const dataset::DataArray &actual,
                     const std::string &optional_message);

struct SCIPP_DATASET_EXPORT DatasetError : public Error<dataset::Dataset> {
  explicit DatasetError(const std::string &msg);
};

template <>
[[noreturn]] SCIPP_DATASET_EXPORT void
throw_mismatch_error(const dataset::Dataset &expected,
                     const dataset::Dataset &actual,
                     const std::string &optional_message);

struct SCIPP_DATASET_EXPORT CoordMismatchError : public DatasetError {
  CoordMismatchError(const Dim dim, const Variable &a, const Variable &b,
                     std::string_view opname = "");
  using DatasetError::DatasetError;
};

} // namespace scipp::except

namespace scipp::dataset::expect {

SCIPP_DATASET_EXPORT void coords_are_superset(const DataArray &a,
                                              const DataArray &b,
                                              std::string_view opname);
SCIPP_DATASET_EXPORT void coords_are_superset(const Coords &a, const Coords &b,
                                              std::string_view opname);
SCIPP_DATASET_EXPORT void matching_coord(const Dim dim, const Variable &a,
                                         const Variable &b,
                                         std::string_view opname);

SCIPP_DATASET_EXPORT void is_key(const Variable &key);

} // namespace scipp::dataset::expect
