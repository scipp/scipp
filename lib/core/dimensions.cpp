// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <numeric>

#include "rename.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/core/sizes.h"

namespace scipp::core {
Dimensions::Dimensions(const std::span<const Dim> labels,
                       const std::span<const scipp::index> shape) {
  if (labels.size() != shape.size())
    throw except::DimensionError(
        "Constructing Dimensions: Number of dimensions "
        "labels (" +
        std::to_string(labels.size()) + ") does not match shape size (" +
        std::to_string(shape.size()) + ").");
  for (scipp::index i = 0; i < scipp::size(shape); ++i)
    addInner(labels[i], shape[i]);
}

Dim Dimensions::label(const scipp::index i) const { return labels()[i]; }

scipp::index Dimensions::size(const scipp::index i) const { return shape()[i]; }

/// Return the offset of elements along this dimension in a multi-dimensional
/// array defined by this.
scipp::index Dimensions::offset(const Dim label) const {
  scipp::index offset{1};
  for (int32_t i = index(label) + 1; i < ndim(); ++i)
    offset *= shape()[i];
  return offset;
}

/// Add a new dimension, which will be the outermost dimension.
void Dimensions::add(const Dim label, const scipp::index size) {
  expect::validDim(label);
  expect::validExtent(size);
  insert_left(label, size);
}

/// Add a new dimension, which will be the innermost dimension.
void Dimensions::addInner(const Dim label, const scipp::index size) {
  expect::validDim(label);
  expect::validExtent(size);
  insert_right(label, size);
}

/// Return the innermost dimension. Returns Dim::Invalid if *this is empty
Dim Dimensions::inner() const noexcept {
  if (empty())
    return Dim::Invalid;
  return labels().back();
}

Dimensions
Dimensions::rename_dims(const std::vector<std::pair<Dim, Dim>> &names,
                        const bool fail_on_unknown) const {
  return detail::rename_dims(*this, names, fail_on_unknown);
}

Dimensions merge(const Dimensions &a) { return a; }

/// Return the direct sum, i.e., the combination of dimensions in a and b.
///
/// Throws if there is a mismatching dimension extent.
/// The implementation "favors" the order of the first argument if both
/// inputs have the same number of dimension. Transposing is avoided where
/// possible, which is crucial for accumulate performance.
Dimensions merge(const Dimensions &a, const Dimensions &b) {
  Dimensions out;
  auto it = b.labels().begin();
  auto end = b.labels().end();
  for (const auto &dim : a.labels()) {
    // add any labels appearing *before* dim
    if (b.contains(dim)) {
      if (a[dim] != b[dim])
        throw except::DimensionError(
            "Cannot merge dimensions with mismatching extent in '" +
            to_string(dim) + "': " + to_string(a) + " and " + to_string(b));
      while (it != end && *it != dim) {
        if (!a.contains(*it))
          out.addInner(*it, b[*it]);
        ++it;
      }
    }
    out.addInner(dim, a[dim]);
  }
  // add remaining labels appearing after last of a's labels
  while (it != end) {
    if (!a.contains(*it))
      out.addInner(*it, b[*it]);
    ++it;
  }
  return out;
}

/// Return the dimensions contained in both a and b (dimension order is not
/// checked).
/// The convention is the same as for merge: we favor the dimension order in a
/// for dimensions found both in a and b.
Dimensions intersection(const Dimensions &a, const Dimensions &b) {
  Dimensions out;
  Dimensions m = merge(a, b);
  for (const auto &dim : m.labels()) {
    if (a.contains(dim) && b.contains(dim))
      out.addInner(dim, m[dim]);
  }
  return out;
}

namespace {
Dimensions transpose_impl(const Dimensions &dims,
                          const std::span<const Dim> labels) {
  if (scipp::size(labels) != dims.ndim())
    throw except::DimensionError("Cannot transpose: Requested new dimension "
                                 "order contains different number of labels.");
  std::vector<scipp::index> shape(labels.size());
  std::transform(labels.begin(), labels.end(), shape.begin(),
                 [&dims](const auto &dim) { return dims[dim]; });
  return {labels, shape};
}
} // namespace

Dimensions transpose(const Dimensions &dims,
                     const std::span<const Dim> labels) {
  if (labels.empty()) {
    std::vector<Dim> default_labels{dims.labels().rbegin(),
                                    dims.labels().rend()};
    return transpose_impl(dims, default_labels);
  }
  return transpose_impl(dims, labels);
}

/// Fold one dim into multiple dims
///
/// Go through the old dims and:
/// - if the dim does not equal the dim that is being stacked, copy dim/shape
/// - if the dim equals the dim to be stacked, replace by stack of new dims
///
/// Note that addInner will protect against inserting new dims that already
/// exist in the old dims.
Dimensions fold(const Dimensions &old_dims, const Dim from_dim,
                const Dimensions &to_dims) {
  scipp::expect::contains(old_dims, from_dim);
  Dimensions new_dims;
  for (const auto &dim : old_dims.labels())
    if (dim != from_dim)
      new_dims.addInner(dim, old_dims[dim]);
    else
      for (const auto &lab : to_dims.labels())
        new_dims.addInner(lab, to_dims[lab]);
  if (old_dims.volume() != new_dims.volume())
    throw except::DimensionError(
        "Sizes " + to_string(to_dims) +
        " provided to `fold` not compatible with length '" +
        std::to_string(old_dims[from_dim]) + "' of dimension '" +
        from_dim.name() + "' being folded.");
  return new_dims;
}

} // namespace scipp::core
