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
#include "scipp/common/except.h"
#include "scipp/common/index.h"
#include "scipp/core/dtype.h"
#include "scipp/units/except.h"
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

} // namespace scipp::core

namespace scipp::except {

struct SCIPP_CORE_EXPORT TypeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

using DataArrayError = Error<core::DataArray>;
using DatasetError = Error<core::Dataset>;
using DimensionError = Error<core::Dimensions>;
using VariableError = Error<core::Variable>;

using DataArrayMismatchError = MismatchError<core::DataArray>;
using DatasetMismatchError = MismatchError<core::Dataset>;
using DimensionMismatchError = MismatchError<core::Dimensions>;
using VariableMismatchError = MismatchError<core::Variable>;

template <class T>
MismatchError(const core::VariableConstProxy &, const T &)
    ->MismatchError<core::Variable>;
template <class T>
MismatchError(const core::DatasetConstProxy &, const T &)
    ->MismatchError<core::Dataset>;
template <class T>
MismatchError(const core::DataConstProxy &, const T &)
    ->MismatchError<core::DataArray>;
template <class T>
MismatchError(const core::Dimensions &, const T &)
    ->MismatchError<core::Dimensions>;

struct SCIPP_CORE_EXPORT DimensionNotFoundError : public DimensionError {
  DimensionNotFoundError(const core::Dimensions &expected, const Dim actual);
};

struct SCIPP_CORE_EXPORT DimensionLengthError : public DimensionError {
  DimensionLengthError(const core::Dimensions &expected, const Dim actual,
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

struct SCIPP_CORE_EXPORT BinEdgeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

} // namespace scipp::except

namespace scipp::core::expect {
template <class A, class B> void equals(const A &a, const B &b) {
  if (a != b)
    throw scipp::except::MismatchError(a, b);
}

template <class A, class Dim, class System, class Enable>
void equals(const A &a, const boost::units::unit<Dim, System, Enable> &unit) {
  const auto expectedUnit = units::Unit(unit);
  if (a != expectedUnit)
    throw scipp::except::MismatchError(a, expectedUnit);
}
template <class A, class Dim, class System, class Enable>
void equals(const boost::units::unit<Dim, System, Enable> &unit, const A &a) {
  equals(a, unit);
}

SCIPP_CORE_EXPORT void dimensionMatches(const Dimensions &dims, const Dim dim,
                                        const scipp::index length);

template <class T, class... Ts>
void sizeMatches(const T &range, const Ts &... other) {
  if (((scipp::size(range) != scipp::size(other)) || ...))
    throw except::SizeError("Expected matching sizes.");
}

inline auto to_string(const std::string &s) { return s; }

template <class A, class B> void contains(const A &a, const B &b) {
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
template <class T> void notSparse(const T &object) { notSparse(object.dims()); }
void SCIPP_CORE_EXPORT validDim(const Dim dim);
void SCIPP_CORE_EXPORT validExtent(const scipp::index size);

} // namespace scipp::core::expect

#endif // EXCEPT_H
