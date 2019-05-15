// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef EXCEPT_H
#define EXCEPT_H

#include <stdexcept>
#include <string>

#include "dimension.h"
#include "dtype.h"
#include "index.h"
#include "scipp/units/unit.h"

namespace scipp::units {
class Unit;
}

namespace scipp::core {

class DataConstProxy;
class DatasetConstProxy;
class Dataset;
class Dimensions;
class Variable;
class ConstVariableSlice;
struct Slice;

std::string to_string(const DType dtype);
std::string to_string(const Dim dim, const std::string &separator = "::");
std::string to_string(const Dimensions &dims,
                      const std::string &separator = "::");
std::string to_string(const Slice &slice, const std::string &separator = "::");
std::string to_string(const units::Unit &unit,
                      const std::string &separator = "::");
std::string to_string(const Variable &variable,
                      const std::string &separator = "::");
std::string to_string(const ConstVariableSlice &variable,
                      const std::string &separator = "::");
std::string to_string(const Dataset &dataset,
                      const std::string &separator = "::");
std::string to_string(const DatasetConstProxy &dataset,
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

struct TypeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct DimensionError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct DimensionMismatchError : public DimensionError {
  DimensionMismatchError(const Dimensions &expected, const Dimensions &actual);
};

struct DimensionNotFoundError : public DimensionError {
  DimensionNotFoundError(const Dimensions &expected, const Dim actual);
};

struct DimensionLengthError : public DimensionError {
  DimensionLengthError(const Dimensions &expected, const Dim actual,
                       const scipp::index length);
};

struct SparseDimensionError : public DimensionError {
  SparseDimensionError()
      : DimensionError("Unsupported operation for sparse dimensions.") {}
};

struct DatasetError : public std::runtime_error {
  DatasetError(const Dataset &dataset, const std::string &message);
  DatasetError(const DatasetConstProxy &dataset, const std::string &message);
};

struct VariableError : public std::runtime_error {
  VariableError(const Variable &variable, const std::string &message);
  VariableError(const ConstVariableSlice &variable, const std::string &message);
};

struct VariableMismatchError : public VariableError {
  template <class A, class B>
  VariableMismatchError(const A &a, const B &b)
      : VariableError(a, "expected to match\n" + to_string(b)) {}
};

struct UnitError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct UnitMismatchError : public UnitError {
  UnitMismatchError(const units::Unit &a, const units::Unit &b);
};

struct SliceError : public std::out_of_range {
  using std::out_of_range::out_of_range;
};

struct CoordMismatchError : public std::runtime_error {
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
  if (!(units::containsCounts(object.unit()) ||
        units::containsCountsVariance(object.unit())))
    throw except::UnitError("Expected counts or counts-density, got " +
                            object.unit().name() + '.');
}

void validSlice(const Dimensions &dims, const Slice &slice);
void coordsAndLabelsMatch(const DataConstProxy &a, const DataConstProxy &b);
void coordsAndLabelsAreSuperset(const DataConstProxy &a,
                                const DataConstProxy &b);

} // namespace expect
} // namespace scipp::core

#endif // EXCEPT_H
