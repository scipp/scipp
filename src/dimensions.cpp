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

/// Returns true if *this forms a contiguous block within parent, i.e.,
/// dimensions are not transposed, missing dimensions are outer dimensions in
/// parent, only the outermost dimensions may be shorter than the corresponding
/// dimension in parent.
bool Dimensions::isContiguousIn(const Dimensions &parent) const {
  if (parent == *this)
    return true;
  int32_t offset = parent.count() - count();
  if (offset < 0)
    return false;
  for (int32_t i = 0; i < count(); ++i) {
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

Dim Dimensions::label(const gsl::index i) const { return m_dims[i]; }

gsl::index Dimensions::size(const gsl::index i) const { return m_shape[i]; }

/// Return the offset of elements along this dimension in a multi-dimensional
/// array defined by this.
gsl::index Dimensions::offset(const Dim label) const {
  gsl::index offset{1};
  for (int32_t i = m_ndim - 1; i >= 0; --i) {
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

void Dimensions::erase(const Dim label) {
  for (int32_t i = index(label); i < m_ndim - 1; ++i) {
    m_shape[i] = m_shape[i + 1];
    m_dims[i] = m_dims[i + 1];
  }
  --m_ndim;
  m_shape[m_ndim] = -1;
  m_dims[m_ndim] = Dim::Invalid;
}

void Dimensions::add(const Dim label, const gsl::index size) {
  if (contains(label))
    throw std::runtime_error("Duplicate dimension.");
  if (m_ndim == 6)
    throw std::runtime_error("More than 6 dimensions are not supported.");
  for (int32_t i = m_ndim - 1; i >= 0; --i) {
    m_shape[i + 1] = m_shape[i];
    m_dims[i + 1] = m_dims[i];
  }
  m_shape[0] = size;
  m_dims[0] = label;
  ++m_ndim;
}

int32_t Dimensions::index(const Dim dim) const {
  for (int32_t i = 0; i < 6; ++i)
    if (m_dims[i] == dim)
      return i;
  throw dataset::except::DimensionNotFoundError(*this, dim);
}
