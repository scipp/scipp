// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "scipp-core_export.h"
#include "scipp/common/except.h"
#include "scipp/common/index.h"
#include "scipp/core/dtype.h"
#include "scipp/core/string.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

namespace scipp::core {

class Dimensions;
class Slice;

} // namespace scipp::core

namespace scipp::except {

struct SCIPP_CORE_EXPORT TypeError : public std::runtime_error {
  using std::runtime_error::runtime_error;

  template <class... Vars>
  TypeError(const std::string &msg) : std::runtime_error(msg) {}

  template <class... Vars>
  TypeError(const std::string &msg, Vars &&... vars)
      : std::runtime_error(msg + ((to_string(vars.dtype()) + ' ') + ...)) {}
};

using DimensionMismatchError = MismatchError<core::Dimensions>;

template <class T>
MismatchError(const core::Dimensions &, const T &)
    -> MismatchError<core::Dimensions>;

using TypeMismatchError = MismatchError<core::DType>;

template <class T>
MismatchError(const core::DType &, const T &) -> MismatchError<core::DType>;

struct SCIPP_CORE_EXPORT DimensionError : public Error<core::Dimensions> {
  DimensionError(const std::string &msg);
  DimensionError(scipp::index expectedDim, scipp::index userDim);
};

struct SCIPP_CORE_EXPORT DimensionNotFoundError : public DimensionError {
  DimensionNotFoundError(const core::Dimensions &expected, const Dim actual);
};

struct SCIPP_CORE_EXPORT DimensionLengthError : public DimensionError {
  DimensionLengthError(const core::Dimensions &expected, const Dim actual,
                       const scipp::index length);
};

struct SCIPP_CORE_EXPORT EventsDimensionError : public DimensionError {
  EventsDimensionError()
      : DimensionError("Unsupported operation for events dimensions.") {}
};

struct SCIPP_CORE_EXPORT SizeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT SliceError : public std::out_of_range {
  using std::out_of_range::out_of_range;
};

struct SCIPP_CORE_EXPORT VariancesError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT BinEdgeError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT NotFoundError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT UnalignedError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT RealignedDataError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct SCIPP_CORE_EXPORT EventDataError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

} // namespace scipp::except

namespace scipp::expect {
template <class A, class B> void contains(const A &a, const B &b) {
  using core::to_string;
  if (!a.contains(b))
    throw except::NotFoundError("Expected " + to_string(a) + " to contain " +
                                to_string(b) + ".");
}
} // namespace scipp::expect

namespace scipp::core::expect {
template <class A, class B> void equals(const A &a, const B &b) {
  if (a != b)
    throw scipp::except::MismatchError(a, b);
}

template <class A, class B>
void equals_any_of(const A &a, const std::initializer_list<B> possible) {
  if (std::find(possible.begin(), possible.end(), a) == possible.end())
    throw scipp::except::MismatchError(a, possible);
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

template <class T> void unit(const T &object, const units::Unit &unit) {
  expect::equals(object.unit(), unit);
}

template <class T>
void unit_any_of(const T &object, std::initializer_list<units::Unit> units) {
  expect::equals_any_of(object.unit(), units);
}

template <class T> void countsOrCountsDensity(const T &object) {
  if (!object.unit().isCounts() && !object.unit().isCountDensity())
    throw except::UnitError("Expected counts or counts-density, got " +
                            object.unit().name() + '.');
}

void SCIPP_CORE_EXPORT validSlice(const Dimensions &dims, const Slice &slice);
void SCIPP_CORE_EXPORT validSlice(
    const std::unordered_map<Dim, scipp::index> &dims, const Slice &slice);

void SCIPP_CORE_EXPORT notCountDensity(const units::Unit &unit);
void SCIPP_CORE_EXPORT validDim(const Dim dim);
void SCIPP_CORE_EXPORT validExtent(const scipp::index size);
template <class T> void hasVariances(const T &variable) {
  if (!variable.hasVariances())
    throw except::VariancesError(to_string(variable) +
                                 " does not have variances.");
}

} // namespace scipp::core::expect
