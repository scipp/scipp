// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/except.h"
#include "scipp/common/index.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/slice.h"

#include <cmath>
#include <set>

namespace scipp::except {

DimensionError::DimensionError(const std::string &msg)
    : Error<core::Dimensions>(msg) {}

DimensionError::DimensionError(scipp::index expectedDim, scipp::index userDim)
    : DimensionError("Length mismatch on insertion. Expected size: " +
                     std::to_string(std::abs(expectedDim)) +
                     " Requested size: " + std::to_string(userDim)) {}

DimensionNotFoundError::DimensionNotFoundError(const core::Dimensions &expected,
                                               const Dim actual)
    : DimensionError("Expected dimension to be a non-events dimension of " +
                     to_string(expected) + ", got " + to_string(actual) + ".") {
}

DimensionLengthError::DimensionLengthError(const core::Dimensions &expected,
                                           const Dim actual,
                                           const scipp::index length)
    : DimensionError("Expected dimension to be in " + to_string(expected) +
                     ", got " + to_string(actual) +
                     " with mismatching length " + std::to_string(length) +
                     ".") {}

} // namespace scipp::except

namespace scipp::core::expect {
void dimensionMatches(const Dimensions &dims, const Dim dim,
                      const scipp::index length) {
  if (dims[dim] != length)
    throw except::DimensionLengthError(dims, dim, length);
}

void validSlice(const Dimensions &dims, const Slice &slice) {
  const auto end = slice.end() < 0 ? slice.begin() + 1 : slice.end();
  if (!dims.contains(slice.dim()) || end > dims[slice.dim()])
    throw except::SliceError("Expected " + to_string(slice) + " to be in " +
                             to_string(dims) + ".");
}
void validSlice(const std::unordered_map<Dim, scipp::index> &dims,
                const Slice &slice) {
  const auto end = slice.end() < 0 ? slice.begin() + 1 : slice.end();
  if (dims.find(slice.dim()) == dims.end() || end > dims.at(slice.dim()))
    throw except::SliceError(
        "Expected " + to_string(slice) +
        " to be in dimensions."); // TODO to_string for map needed
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
