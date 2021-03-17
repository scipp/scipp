// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <numeric>

#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"

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
scipp::index Dimensions::operator[](const Dim dim) const { return at(dim); }

/// Return the extent of `dim`. Throws if the space defined by this does not
/// contain `dim`.
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

/// Return true if all dimensions of other contained in *this, ignoring order.
bool Dimensions::contains(const Dimensions &other) const noexcept {
  if (*this == other)
    return true;
  for (const auto &dim : other.labels())
    if (!contains(dim) || other[dim] != operator[](dim))
      return false;
  return true;
}

/// Return true if *this forms a contiguous block within parent.
///
/// Specifically, dimensions are not transposed, missing dimensions are outer
/// dimensions in parent, and only the outermost dimensions may be shorter than
/// the corresponding dimension in parent.
bool Dimensions::isContiguousIn(const Dimensions &parent) const {
  if (parent == *this)
    return true;
  int32_t offset = parent.m_ndim - m_ndim;
  if (offset < 0)
    return false;
  for (int32_t i = 0; i < m_ndim; ++i) {
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

Dim Dimensions::label(const scipp::index i) const { return m_dims[i]; }

void Dimensions::relabel(const scipp::index i, const Dim label) {
  if (label != Dim::Invalid)
    expectUnique(*this, label);
  m_dims[i] = label;
}

scipp::index Dimensions::size(const scipp::index i) const { return m_shape[i]; }

/// Return the offset of elements along this dimension in a multi-dimensional
/// array defined by this.
scipp::index Dimensions::offset(const Dim label) const {
  scipp::index offset{1};
  for (int32_t i = m_ndim - 1; i >= 0; --i) {
    if (m_dims[i] == label)
      return offset;
    offset *= m_shape[i];
  }
  except::throw_dimension_not_found_error(*this, label);
}

void Dimensions::resize(const Dim label, const scipp::index size) {
  expect::validExtent(size);
  at(label) = size;
}

void Dimensions::resize(const scipp::index i, const scipp::index size) {
  resize(label(i), size);
}

void Dimensions::erase(const Dim label) {
  for (int32_t i = index(label); i < m_ndim - 1; ++i) {
    m_shape[i] = m_shape[i + 1];
    m_dims[i] = m_dims[i + 1];
  }
  m_dims[m_ndim - 1] = m_dims[m_ndim];
  m_dims[m_ndim] = Dim::Invalid;
  --m_ndim;
  m_shape[m_ndim] = -1;
}

/// Add a new dimension, which will be the outermost dimension.
void Dimensions::add(const Dim label, const scipp::index size) {
  expect::validDim(label);
  expectUnique(*this, label);
  expectExtendable(*this);
  expect::validExtent(size);
  m_dims[m_ndim + 1] = m_dims[m_ndim];
  for (int32_t i = m_ndim - 1; i >= 0; --i) {
    m_shape[i + 1] = m_shape[i];
    m_dims[i + 1] = m_dims[i];
  }
  m_shape[0] = size;
  m_dims[0] = label;
  ++m_ndim;
}

/// Add a new dimension, which will be the innermost dimension.
void Dimensions::addInner(const Dim label, const scipp::index size) {
  expect::validDim(label);
  expectUnique(*this, label);
  expect::validExtent(size);
  expectExtendable(*this);
  m_shape[m_ndim] = size;
  m_dims[m_ndim] = label;
  ++m_ndim;
}

/// Return the innermost dimension. Throws if *this is empty.
Dim Dimensions::inner() const noexcept {
  if (m_ndim == 0)
    return Dim::Invalid;
  return m_dims[m_ndim - 1];
}

int32_t Dimensions::index(const Dim dim) const {
  expect::validDim(dim);
  for (int32_t i = 0; i < NDIM_MAX; ++i)
    if (m_dims[i] == dim)
      return i;
  except::throw_dimension_not_found_error(*this, dim);
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

Dimensions transpose(const Dimensions &dims, std::vector<Dim> labels) {
  if (labels.empty())
    labels.insert(labels.end(), dims.labels().rbegin(), dims.labels().rend());
  else if (labels.size() != dims.ndim())
    throw except::DimensionError("Cannot transpose: Requested new dimension "
                                 "order contains different number of labels.");
  std::vector<scipp::index> shape(labels.size());
  std::transform(labels.begin(), labels.end(), shape.begin(),
                 [&dims](auto &dim) { return dims[dim]; });
  return {labels, shape};
}

/// Fold one dim into multiple dims
///
/// Go through the old dims and:
/// - if the dim does not equal the dim that is being stacked, copy dim/shape
/// - if the dim equals the dim to be stacked, replace by stack of new dims
///
/// Note that addInner will protect against inserting new dims that already
/// exist in the old dims.
/// If from_dim is not found in old_dims, the new dims are identical to the
/// old_dims (this occurs when folding a DataArray whose coordinates do not all
/// necessarily contain from_dim).
Dimensions fold(const Dimensions &old_dims, const Dim from_dim,
                const Dimensions &to_dims) {
  Dimensions new_dims;
  for (const auto &dim : old_dims.labels())
    if (dim != from_dim)
      new_dims.addInner(dim, old_dims[dim]);
    else
      for (const auto &lab : to_dims.labels())
        new_dims.addInner(lab, to_dims[lab]);
  return new_dims;
}

/// Flatten multiple dims into one
///
/// Go through the old dims and:
/// - if the dim is contained in the list of dims to be flattened, add the new
///   dim once
/// - if not, copy the dim/shape
///
/// Note that from_dims are not necessarily present in old_dims, which allows
/// to silently skip flattening variables that do not depend on from_labels.
Dimensions flatten(const Dimensions &old_dims,
                   const scipp::span<const Dim> from_labels, const Dim to_dim) {
  Dimensions from_dims;
  for (const auto &dim : from_labels)
    if (old_dims.contains(dim))
      from_dims.addInner(dim, old_dims[dim]);

  // Only allow reshaping contiguous dimensions.
  // We check that the intersection of old_dims and from_dims is found as a
  // contiguous block with the correct order inside both old_dims and from_dims.
  Dimensions intersect = intersection(old_dims, from_dims);
  for (scipp::index i = 0; i < intersect.ndim() - 1; ++i)
    if (old_dims.index(intersect.label(i + 1)) !=
            old_dims.index(intersect.label(i)) + 1 ||
        from_dims.index(intersect.label(i + 1)) !=
            from_dims.index(intersect.label(i)) + 1)
      throw except::DimensionError("Can only flatten a contiguous set of "
                                   "dimensions in the correct order");

  Dimensions new_dims;
  for (const auto &dim : old_dims.labels())
    if (from_dims.contains(dim)) {
      if (!new_dims.contains(to_dim))
        new_dims.addInner(to_dim, from_dims.volume());
    } else {
      new_dims.addInner(dim, old_dims[dim]);
    }
  return new_dims;
}

} // namespace scipp::core
