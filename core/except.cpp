// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/except.h"
#include "scipp/common/index.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/slice.h"

namespace scipp::except {

TypeError::TypeError(const std::string &msg) : Error{msg} {}

template <>
void throw_mismatch_error(const core::DType &expected,
                          const core::DType &actual,
                          const std::string &optional_message) {
  throw TypeError("Expected dtype " + to_string(expected) + ", got " +
                  to_string(actual) + '.' + optional_message);
}

DimensionError::DimensionError(const std::string &msg)
    : Error<core::Dimensions>(msg) {}

DimensionError::DimensionError(scipp::index expectedDim, scipp::index userDim)
    : DimensionError("Length mismatch on insertion. Expected size: " +
                     std::to_string(std::abs(expectedDim)) +
                     " Requested size: " + std::to_string(userDim)) {}

namespace {
template <class T> std::string format_dims(const T &dims) {
  if (dims.empty()) {
    return "a scalar";
  }
  return "dimensions " + to_string(dims);
}
} // namespace

template <>
void throw_mismatch_error(const core::Sizes &expected,
                          const core::Sizes &actual,
                          const std::string &optional_message) {
  throw DimensionError("Expected " + format_dims(expected) + ", got " +
                       format_dims(actual) + '.' + optional_message);
}

template <>
void throw_mismatch_error(const core::Dimensions &expected,
                          const core::Dimensions &actual,
                          const std::string &optional_message) {
  throw DimensionError("Expected " + format_dims(expected) + ", got " +
                       format_dims(actual) + '.' + optional_message);
}

void throw_dimension_length_error(const core::Dimensions &expected, Dim actual,
                                  index length) {
  throw DimensionError{"Expected dimension to be in " + to_string(expected) +
                       ", got " + to_string(actual) +
                       " with mismatching length " + std::to_string(length) +
                       '.'};
}

void throw_cannot_have_variances(const DType type) {
  throw except::VariancesError("Variances for dtype=" + to_string(type) +
                               " not supported.");
}

} // namespace scipp::except

namespace scipp::core::expect {
void dimensionMatches(const Dimensions &dims, const Dim dim,
                      const scipp::index length) {
  if (dims[dim] != length)
    except::throw_dimension_length_error(dims, dim, length);
}

void validSlice(const Sizes &dims, const Slice &slice) {
  if (slice == Slice{})
    return;
  const auto end = slice.end() < 0 ? slice.begin() + 1 : slice.end();
  if (!dims.contains(slice.dim()) || end > dims[slice.dim()])
    throw except::SliceError("Expected " + to_string(slice) + " to be in " +
                             to_string(dims) + ".");
}

void notCountDensity(const units::Unit &unit) {
  if (unit.isCountDensity())
    throw except::UnitError("Expected non-count-density unit.");
}

void validDim(const Dim dim) {
  if (dim == Dim::Invalid)
    throw except::DimensionError("Dim::Invalid is not a valid dimension.");
}

void validExtent(const scipp::index size) {
  if (size < 0)
    throw except::DimensionError("Dimension size cannot be negative.");
}

} // namespace scipp::core::expect
