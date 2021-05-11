// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <numeric>

#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/core/sizes.h"

namespace scipp::core {

void expectUnique(const Dimensions &dims, const Dim label) {
  if (dims.contains(label))
    throw except::DimensionError("Duplicate dimension.");
}

void expectExtendable(const Dimensions &dims) {
  if (dims.shape().size() == NDIM_MAX)
    throw except::DimensionError(
        "Maximum number of allowed dimensions exceeded.");
}

Dimensions::Dimensions(const std::vector<Dim> &labels,
                       const std::vector<scipp::index> &shape) {
  if (labels.size() != shape.size())
    throw except::DimensionError(
        "Constructing Dimensions: Number of dimensions "
        "labels (" +
        std::to_string(labels.size()) + ") does not match shape size (" +
        std::to_string(shape.size()) + ").");
  for (scipp::index i = 0; i < scipp::size(shape); ++i)
    addInner(labels[i], shape[i]);
}

/// Return the extent of `dim`. Throws if the space defined by this does not
/// contain `dim`.
// scipp::index Dimensions::operator[](const Dim dim) const { return at(dim); }

/// Return the extent of `dim`. Throws if the space defined by this does not
/// contain `dim`.
/*
scipp::index Dimensions::at(const Dim dim) const {
  for (int32_t i = 0; i < m_ndim; ++i)
    if (m_dims[i] == dim)
      return m_shape[i];
  except::throw_dimension_not_found_error(*this, dim);
}

/// Return a mutable reference to the extent of `dim`. Throws if the space
/// defined by this does not contain `dim`.
scipp::index &Dimensions::at(const Dim dim) {
  for (int32_t i = 0; i < m_ndim; ++i)
    if (m_dims[i] == dim)
      return m_shape[i];
  except::throw_dimension_not_found_error(*this, dim);
}
*/

/// Return true if *this forms a contiguous block within parent.
///
/// Specifically, dimensions are not transposed, missing dimensions are outer
/// dimensions in parent, and only the outermost dimensions may be shorter than
/// the corresponding dimension in parent.
bool Dimensions::isContiguousIn(const Dimensions &parent) const {
  if (volume() == 0 || parent == *this)
    return true;
  int32_t offset = parent.ndim() - ndim();
  if (offset < 0)
    return false;
  for (int32_t i = 0; i < ndim(); ++i) {
    // All shared dimension labels must match.
    if (parent.label(i + offset) != label(i))
      return false;
    // Outermost dimension of *this can be a section of parent.
    // All but outermost must match.
    if (i == 0) {
      if (parent.size(offset) < size(0))
        return false;
    } else if (parent.size(i + offset) != size(i)) {
      return false;
    }
  }
  return true;
}

Dim Dimensions::label(const scipp::index i) const { return labels()[i]; }

void Dimensions::relabel(const scipp::index i, const Dim label) {
  replace_key(labels()[i], label);
}

scipp::index Dimensions::size(const scipp::index i) const { return shape()[i]; }

/// Return the offset of elements along this dimension in a multi-dimensional
/// array defined by this.
scipp::index Dimensions::offset(const Dim label) const {
  scipp::index offset{1};
  for (int32_t i = index(label) + 1; i < ndim(); ++i)
    offset *= shape()[i];
  return offset;
}

void Dimensions::resize(const Dim label, const scipp::index size) {
  expect::validExtent(size);
  at(label) = size;
}

void Dimensions::resize(const scipp::index i, const scipp::index size) {
  resize(label(i), size);
}

/// Add a new dimension, which will be the outermost dimension.
void Dimensions::add(const Dim label, const scipp::index size) {
  expect::validDim(label);
  expectUnique(*this, label);
  expectExtendable(*this);
  expect::validExtent(size);
  insert_left(label, size);
}

/// Add a new dimension, which will be the innermost dimension.
void Dimensions::addInner(const Dim label, const scipp::index size) {
  expect::validDim(label);
  expectUnique(*this, label);
  expect::validExtent(size);
  expectExtendable(*this);
  insert_right(label, size);
}

/// Return the innermost dimension. Returns Dim::Invalid if *this is empty
Dim Dimensions::inner() const noexcept {
  if (empty())
    return Dim::Invalid;
  return labels().back();
}

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
            "Cannot merge subspaces with mismatching extent");
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
                          const std::vector<Dim> &labels) {
  if (scipp::size(labels) != dims.ndim())
    throw except::DimensionError("Cannot transpose: Requested new dimension "
                                 "order contains different number of labels.");
  std::vector<scipp::index> shape(labels.size());
  std::transform(labels.begin(), labels.end(), shape.begin(),
                 [&dims](auto &dim) { return dims[dim]; });
  return {labels, shape};
}
} // namespace

Dimensions transpose(const Dimensions &dims, const std::vector<Dim> &labels) {
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
