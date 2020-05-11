// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
        "labels does not match shape.");
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
  throw except::DimensionNotFoundError(*this, dim);
}

/// Return a mutable reference to the extent of `dim`. Throws if the space
/// defined by this does not contain `dim`.
scipp::index &Dimensions::at(const Dim dim) {
  for (int32_t i = 0; i < m_ndim; ++i)
    if (m_dims[i] == dim)
      return m_shape[i];
  throw except::DimensionNotFoundError(*this, dim);
}

/// Return true if all dimensions of other contained in *this, ignoring order.
bool Dimensions::contains(const Dimensions &other) const noexcept {
  if (*this == other)
    return true;
  for (const auto dim : other.labels())
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
  throw except::DimensionNotFoundError(*this, label);
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
  throw except::DimensionNotFoundError(*this, dim);
}

/// Return the direct sum, i.e., the combination of dimensions in a and b.
///
/// Throws if there is a mismatching dimension extent.
Dimensions merge(const Dimensions &a, const Dimensions &b) {
  auto out(a);
  for (const auto dim : b.labels()) {
    if (a.contains(dim)) {
      if (a[dim] != b[dim])
        throw except::DimensionError(
            "Cannot merge subspaces with mismatching extent");
    } else {
      out.addInner(dim, b[dim]);
    }
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

} // namespace scipp::core
