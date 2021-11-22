// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>

#include "scipp-core_export.h"
#include "scipp/common/except.h"
#include "scipp/common/index.h"
#include "scipp/core/dtype.h"
#include "scipp/core/string.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

namespace scipp::core {

class Dimensions;
class Sizes;
class Slice;

} // namespace scipp::core

namespace scipp::except {

struct SCIPP_CORE_EXPORT TypeError : public Error<core::DType> {
  explicit TypeError(const std::string &msg);

  template <class... Vars>
  explicit TypeError(const std::string &msg, Vars &&... vars)
      : TypeError{msg + (('\'' + pretty_dtype(vars) + "', ") + ...)} {}
};

template <>
[[noreturn]] SCIPP_CORE_EXPORT void
throw_mismatch_error(const core::DType &expected, const core::DType &actual,
                     const std::string &optional_message);

struct SCIPP_CORE_EXPORT DimensionError : public Error<core::Dimensions> {
  explicit DimensionError(const std::string &msg);
  DimensionError(scipp::index expectedDim, scipp::index userDim);
};

template <>
[[noreturn]] SCIPP_CORE_EXPORT void
throw_mismatch_error(const core::Sizes &expected, const core::Sizes &actual,
                     const std::string &optional_message);

template <>
[[noreturn]] SCIPP_CORE_EXPORT void
throw_mismatch_error(const core::Dimensions &expected,
                     const core::Dimensions &actual,
                     const std::string &optional_message);

[[noreturn]] SCIPP_CORE_EXPORT void
throw_dimension_length_error(const core::Dimensions &expected, Dim actual,
                             scipp::index length);

struct SCIPP_CORE_EXPORT BinnedDataError : public std::runtime_error {
  using std::runtime_error::runtime_error;
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

[[noreturn]] SCIPP_CORE_EXPORT void
throw_cannot_have_variances(const DType type);

} // namespace scipp::except

namespace scipp::expect {
template <class A, class B> void contains(const A &a, const B &b) {
  using core::to_string;
  if (!a.contains(b))
    throw except::NotFoundError("Expected " + to_string(a) + " to contain " +
                                to_string(b) + ".");
}
template <class A, class B> void includes(const A &a, const B &b) {
  using core::to_string;
  if (!a.includes(b))
    throw except::NotFoundError("Expected " + to_string(a) + " to include " +
                                to_string(b) + ".");
}
} // namespace scipp::expect

namespace scipp::core::expect {
template <class A, class B>
void equals(const A &a, const B &b, std::string optional_message = "") {
  if (a != b)
    scipp::except::throw_mismatch_error(a, b, optional_message);
}

template <class T, class... Ts>
void sizeMatches(const T &range, const Ts &... other) {
  if (((scipp::size(range) != scipp::size(other)) || ...))
    throw except::SizeError("Expected matching sizes.");
}

inline auto to_string(const std::string &s) { return s; }

template <class T>
void unit(const T &object, const units::Unit &unit,
          std::string optional_message = "") {
  expect::equals(object.unit(), unit, optional_message);
}

void SCIPP_CORE_EXPORT ndim_is(const Dimensions &dims, scipp::index expected);

// TODO maybe just provide a `slice` function/method and check via that?
void SCIPP_CORE_EXPORT validSlice(const Sizes &sizes, const Slice &slice);

void SCIPP_CORE_EXPORT validDim(const Dim dim);
void SCIPP_CORE_EXPORT validExtent(const scipp::index size);
template <class T> void canHaveVariances() {
  if (!core::canHaveVariances<T>())
    except::throw_cannot_have_variances(dtype<T>);
}

} // namespace scipp::core::expect
