/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <algorithm>
#include <numeric>

#include "dimensions.h"
#include "variable.h"

/// Returns true if all dimensions of other are also contained in *this. Does
/// not check dimension order.
bool Dimensions::contains(const Dimensions &other) const {
  if (*this == other)
    return true;
  for (const auto dim : other.labels())
    if (!contains(dim))
      return false;
  for (int32_t i = 0; i < other.ndim(); ++i)
    if (other.shape()[i] != operator[](other.labels()[i]))
      return false;
  return true;
}

Dimension Dimensions::label(const gsl::index i) const { return m_dims[i]; }

gsl::index Dimensions::size(const gsl::index i) const { return m_shape[i]; }

gsl::index Dimensions::size(const Dimension label) const {
  return operator[](label);
}

/// Return the offset of elements along this dimension in a multi-dimensional
/// array defined by this.
gsl::index Dimensions::offset(const Dimension label) const {
  gsl::index offset{1};
  for (int32_t i = 0; i < m_ndim; ++i) {
    if (m_dims[i] == label)
      return offset;
    offset *= m_shape[i];
  }
  throw dataset::except::DimensionNotFoundError(*this, label);
}

void Dimensions::resize(const Dim label, const gsl::index size) {
  if (size < 0)
    throw std::runtime_error("Dimension size cannot be negative.");
  operator[](label) = size;
}

void Dimensions::resize(const gsl::index i, const gsl::index size) {
  if (size < 0)
    throw std::runtime_error("Dimension size cannot be negative.");
  m_shape[i] = size;
}

void Dimensions::erase(const Dimension label) {
  for (int32_t i = index(label); i < m_ndim - 1; ++i) {
    m_shape[i] = m_shape[i + 1];
    m_dims[i] = m_dims[i + 1];
  }
  --m_ndim;
  m_shape[m_ndim] = -1;
  m_dims[m_ndim] = Dim::Invalid;
}

void Dimensions::add(const Dimension label, const gsl::index size) {
  if (m_ndim == 6)
    throw std::runtime_error("More than 6 dimensions are not supported.");
  // TODO check duplicate dimensions
  m_shape[m_ndim] = size;
  m_dims[m_ndim] = label;
  ++m_ndim;
}

gsl::index Dimensions::index(const Dim dim) const {
  for (int32_t i = 0; i < 6; ++i)
    if (m_dims[i] == dim)
      return i;
  throw dataset::except::DimensionNotFoundError(*this, dim);
}

Dimensions concatenate(const Dimension dim, const Dimensions &dims1,
                       const Dimensions &dims2) {
  if (dims1.contains(dim) && dims2.contains(dim)) {
    // - all dimension labels must match and have same order.
    // - if dim is ragged, all other dimensions must have matching size.
    // - if dim is not ragged, one other dimension can have size mismatch
    // (create ragged)
  } else {
    // - all dimension labels must match and have same order
    // - in the result, up to one dimension may be ragged
    // - if a dim is ragged, concatenate also ragged sizes
    // - some more failure cases here
  }
}
