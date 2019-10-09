// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <set>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/core/slice.h"

namespace scipp::except {

DimensionNotFoundError::DimensionNotFoundError(const core::Dimensions &expected,
                                               const Dim actual)
    : DimensionError("Expected dimension to be a non-sparse dimension of " +
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
  if (!dims.contains(slice.dim()) || slice.begin() < 0 ||
      slice.begin() >=
          std::min(slice.end() >= 0 ? slice.end() + 1 : dims[slice.dim()],
                   dims[slice.dim()]) ||
      slice.end() > dims[slice.dim()])
    throw except::SliceError("Expected " + to_string(slice) + " to be in " +
                             to_string(dims) + ".");
}
void validSlice(const std::unordered_map<Dim, scipp::index> &dims,
                const Slice &slice) {
  if (dims.find(slice.dim()) == dims.end() || slice.begin() < 0 ||
      slice.begin() >=
          std::min(slice.end() >= 0 ? slice.end() + 1 : dims.at(slice.dim()),
                   dims.at(slice.dim())) ||
      slice.end() > dims.at(slice.dim()))
    throw except::SliceError(
        "Expected " + to_string(slice) +
        " to be in dimensions."); // TODO to_string for map needed
}

void coordsAndLabelsAreSuperset(const DataConstProxy &a,
                                const DataConstProxy &b) {
  for (const auto &[dim, coord] : b.coords())
    if (a.coords()[dim] != coord)
      throw except::CoordMismatchError("Expected coords to match.");
  for (const auto &[name, labels] : b.labels())
    if (a.labels()[name] != labels)
      throw except::CoordMismatchError("Expected labels to match.");
}

void notSparse(const Dimensions &dims) {
  if (dims.sparse())
    throw except::DimensionError("Expected non-sparse dimensions.");
}

void validDim(const Dim dim) {
  if (dim == Dim::Invalid)
    throw except::DimensionError("Dim::Invalid is not a valid dimension.");
}

void validExtent(const scipp::index size) {
  if (size == Dimensions::Sparse)
    throw except::DimensionError("Expected non-sparse dimension extent.");
  if (size < 0)
    throw except::DimensionError("Dimension size cannot be negative.");
}

} // namespace scipp::core::expect
