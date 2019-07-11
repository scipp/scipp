// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef EXCEPT_H
#define EXCEPT_H

#include <stdexcept>
#include <string>

#include "scipp/core/dll_config.h"
#include "scipp/core/dtype.h"
#include "scipp/core/index.h"
#include "scipp/units/unit.h"

namespace scipp::core {

class DataConstProxy;
class DatasetConstProxy;
class Dataset;
class Dimensions;
class Variable;
class VariableConstProxy;
struct Slice;

SCIPP_CORE_DLL std::string to_string(const DType dtype);
SCIPP_CORE_DLL std::string to_string(const Dimensions &dims,
                                     const std::string &separator = "::");
SCIPP_CORE_DLL std::string to_string(const Slice &slice,
                                     const std::string &separator = "::");
SCIPP_CORE_DLL std::string to_string(const units::Unit &unit,
                                     const std::string &separator = "::");
SCIPP_CORE_DLL std::string to_string(const Variable &variable,
                                     const std::string &separator = "::");
SCIPP_CORE_DLL std::string to_string(const VariableConstProxy &variable,
                                     const std::string &separator = "::");
SCIPP_CORE_DLL std::string to_string(const Dataset &dataset,
                                     const std::string &separator = "::");
SCIPP_CORE_DLL std::string to_string(const DatasetConstProxy &dataset,
                                     const std::string &separator = "::");

template <class T> std::string element_to_string(const T &item) {
  using std::to_string;
  if constexpr (std::is_same_v<T, std::string>)
    return {'"' + item + "\", "};
  else if constexpr (std::is_same_v<T, Eigen::Vector3d>)
    return {"(Eigen::Vector3d), "};
  else if constexpr (std::is_same_v<T,
                                    boost::container::small_vector<double, 8>>)
    return {"(vector), "};
  else
    return to_string(item) + ", ";
}

template <class T> std::string array_to_string(const T &arr) {
  const scipp::index size = arr.size();
  if (size == 0)
    return std::string("[]");
  std::string s = "[";
  for (scipp::index i = 0; i < arr.size(); ++i) {
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

struct SCIPP_CORE_DLL TypeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_DLL DimensionError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_DLL DimensionMismatchError : public DimensionError {
  DimensionMismatchError(const Dimensions &expected, const Dimensions &actual);
};

struct SCIPP_CORE_DLL DimensionNotFoundError : public DimensionError {
  DimensionNotFoundError(const Dimensions &expected, const Dim actual);
};

struct SCIPP_CORE_DLL DimensionLengthError : public DimensionError {
  DimensionLengthError(const Dimensions &expected, const Dim actual,
                       const scipp::index length);
};

struct SCIPP_CORE_DLL SparseDimensionError : public DimensionError {
  SparseDimensionError()
      : DimensionError("Unsupported operation for sparse dimensions.") {}
};

struct SCIPP_CORE_DLL DatasetError : public std::runtime_error {
  DatasetError(const Dataset &dataset, const std::string &message);
  DatasetError(const DatasetConstProxy &dataset, const std::string &message);
};

struct SCIPP_CORE_DLL VariableError : public std::runtime_error {
  VariableError(const Variable &variable, const std::string &message);
  VariableError(const VariableConstProxy &variable, const std::string &message);
};

struct SCIPP_CORE_DLL VariableMismatchError : public VariableError {
  template <class A, class B>
  VariableMismatchError(const A &a, const B &b)
      : VariableError(a, "expected to match\n" + to_string(b)) {}
};

struct SCIPP_CORE_DLL UnitError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_DLL SizeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_DLL UnitMismatchError : public UnitError {
  UnitMismatchError(const units::Unit &a, const units::Unit &b);
};

struct SCIPP_CORE_DLL SliceError : public std::out_of_range {
  using std::out_of_range::out_of_range;
};

struct SCIPP_CORE_DLL CoordMismatchError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_DLL VariancesError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

} // namespace except

namespace expect {
template <class A, class B> void variablesMatch(const A &a, const B &b) {
  if (a != b)
    throw except::VariableMismatchError(a, b);
}
void dimensionMatches(const Dimensions &dims, const Dim dim,
                      const scipp::index length);

template <class T, class... Ts>
void sizeMatches(const T &range, const Ts &... other) {
  if (((scipp::size(range) != scipp::size(other)) || ...))
    throw except::SizeError("Expected matching sizes.");
}
void equals(const units::Unit &a, const units::Unit &b);
void equals(const Dimensions &a, const Dimensions &b);

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

void SCIPP_CORE_DLL validSlice(const Dimensions &dims, const Slice &slice);
void SCIPP_CORE_DLL coordsAndLabelsMatch(const DataConstProxy &a,
                                         const DataConstProxy &b);
void SCIPP_CORE_DLL coordsAndLabelsAreSuperset(const DataConstProxy &a,
                                               const DataConstProxy &b);
void SCIPP_CORE_DLL notSparse(const Dimensions &dims);
void SCIPP_CORE_DLL validDim(const Dim dim);
void SCIPP_CORE_DLL validExtent(const scipp::index size);

} // namespace expect
} // namespace scipp::core

#endif // EXCEPT_H
