// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef EXCEPT_H
#define EXCEPT_H

#include <stdexcept>
#include <string>
#include <unordered_map>

#include "scipp-core_export.h"
#include "scipp/common/index.h"
#include "scipp/core/dtype.h"
#include "scipp/units/unit.h"

namespace scipp::core {

class DataConstProxy;
class DatasetConstProxy;
class Dataset;
class DataArray;
class Dimensions;
class Variable;
class VariableConstProxy;
class Slice;

SCIPP_CORE_EXPORT std::string to_string(const DType dtype);
SCIPP_CORE_EXPORT std::string to_string(const Dimensions &dims);
SCIPP_CORE_EXPORT std::string to_string(const Slice &slice);
SCIPP_CORE_EXPORT std::string to_string(const units::Unit &unit);
SCIPP_CORE_EXPORT std::string to_string(const Variable &variable);
SCIPP_CORE_EXPORT std::string to_string(const VariableConstProxy &variable);
SCIPP_CORE_EXPORT std::string to_string(const DataArray &data);
SCIPP_CORE_EXPORT std::string to_string(const DataConstProxy &data);
SCIPP_CORE_EXPORT std::string to_string(const Dataset &dataset);
SCIPP_CORE_EXPORT std::string to_string(const DatasetConstProxy &dataset);

template <class T> std::string array_to_string(const T &arr);

template <class T> std::string element_to_string(const T &item) {
  using std::to_string;
  if constexpr (std::is_same_v<T, std::string>)
    return {'"' + item + "\", "};
  else if constexpr (std::is_same_v<T, Eigen::Vector3d>)
    return {"(" + to_string(item[0]) + ", " + to_string(item[1]) + ", " +
            to_string(item[2]) + "), "};
  else if constexpr (is_sparse_v<T>)
    return array_to_string(item) + ", ";
  else if constexpr (std::is_same_v<T, Dataset>)
    return {"Dataset, "};
  else
    return to_string(item) + ", ";
}

template <class T> std::string array_to_string(const T &arr) {
  const auto size = scipp::size(arr);
  if (size == 0)
    return std::string("[]");
  std::string s = "[";
  for (scipp::index i = 0; i < scipp::size(arr); ++i) {
    if (i == 4 && size > 8) {
      s += "..., ";
      i = size - 4;
    }
    s += element_to_string(arr[i]);
  }
  s.resize(s.size() - 2);
  s += "]";
  return s;
}

namespace except {

struct SCIPP_CORE_EXPORT TypeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

template <class T> struct Error : public std::runtime_error {
  using std::runtime_error::runtime_error;
  template <class T2>
  Error(const T2 &object, const std::string &message)
      : std::runtime_error(to_string(object) + message) {}
};

using DataArrayError = Error<DataArray>;
using DatasetError = Error<Dataset>;
using DimensionError = Error<Dimensions>;
using UnitError = Error<units::Unit>;
using VariableError = Error<Variable>;

template <class T> struct MismatchError : public Error<T> {
  template <class A, class B>
  MismatchError(const A &a, const B &b)
      : Error<T>(a, " expected to be equal to " + to_string(b)) {}
};

using DataArrayMismatchError = MismatchError<DataArray>;
using DatasetMismatchError = MismatchError<Dataset>;
using DimensionMismatchError = MismatchError<Dimensions>;
using UnitMismatchError = MismatchError<units::Unit>;
using VariableMismatchError = MismatchError<Variable>;

// We need deduction guides such that, e.g., the exception for a Variable
// mismatch and VariableProxy mismatch are the same type.
template <class T>
MismatchError(const units::Unit &, const T &)->MismatchError<units::Unit>;
template <class T>
MismatchError(const VariableConstProxy &, const T &)->MismatchError<Variable>;
template <class T>
MismatchError(const DatasetConstProxy &, const T &)->MismatchError<Dataset>;
template <class T>
MismatchError(const DataConstProxy &, const T &)->MismatchError<DataArray>;
template <class T>
MismatchError(const Dimensions &, const T &)->MismatchError<Dimensions>;

struct SCIPP_CORE_EXPORT DimensionNotFoundError : public DimensionError {
  DimensionNotFoundError(const Dimensions &expected, const Dim actual);
};

struct SCIPP_CORE_EXPORT DimensionLengthError : public DimensionError {
  DimensionLengthError(const Dimensions &expected, const Dim actual,
                       const scipp::index length);
};

struct SCIPP_CORE_EXPORT SparseDimensionError : public DimensionError {
  SparseDimensionError()
      : DimensionError("Unsupported operation for sparse dimensions.") {}
};

struct SCIPP_CORE_EXPORT SizeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT SliceError : public std::out_of_range {
  using std::out_of_range::out_of_range;
};

struct SCIPP_CORE_EXPORT CoordMismatchError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT VariancesError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT SparseDataError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

} // namespace except

namespace expect {
template <class A, class B> void equals(const A &a, const B &b) {
  if (a != b)
    throw except::MismatchError(a, b);
}

SCIPP_CORE_EXPORT void dimensionMatches(const Dimensions &dims, const Dim dim,
                                        const scipp::index length);

template <class T, class... Ts>
void sizeMatches(const T &range, const Ts &... other) {
  if (((scipp::size(range) != scipp::size(other)) || ...))
    throw except::SizeError("Expected matching sizes.");
}

template <class T> void contains(const T &a, const T &b) {
  if (!a.contains(b))
    throw std::runtime_error("Expected " + to_string(a) + " to contain " +
                             to_string(b) + ".");
}
template <class T> void unit(const T &object, const units::Unit &unit) {
  expect::equals(object.unit(), unit);
}

template <class T> void countsOrCountsDensity(const T &object) {
  if (!object.unit().isCounts() && !object.unit().isCountDensity())
    throw except::UnitError("Expected counts or counts-density, got " +
                            object.unit().name() + '.');
}

void SCIPP_CORE_EXPORT validSlice(const Dimensions &dims, const Slice &slice);
void SCIPP_CORE_EXPORT validSlice(
    const std::unordered_map<Dim, scipp::index> &dims, const Slice &slice);

void SCIPP_CORE_EXPORT coordsAndLabelsAreSuperset(const DataConstProxy &a,
                                                  const DataConstProxy &b);
void SCIPP_CORE_EXPORT notSparse(const Dimensions &dims);
void SCIPP_CORE_EXPORT validDim(const Dim dim);
void SCIPP_CORE_EXPORT validExtent(const scipp::index size);

} // namespace expect
} // namespace scipp::core

#endif // EXCEPT_H
