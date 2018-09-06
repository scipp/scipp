/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <algorithm>
#include <numeric>

#include "dimensions.h"
#include "variable.h"

Dimensions::Dimensions() = default;
Dimensions::Dimensions(const Dimension label, const gsl::index size) {
  add(label, size);
}
Dimensions::Dimensions(
    const std::vector<std::pair<Dimension, gsl::index>> &sizes) {
  for (const auto &item : sizes)
    add(item.first, item.second);
}

bool Dimensions::operator==(const Dimensions &other) const {
  return m_dims == other.m_dims;
}

gsl::index Dimensions::count() const { return m_dims.size(); }

gsl::index Dimensions::volume() const {
  gsl::index volume{1};
  for (gsl::index dim = 0; dim < count(); ++dim)
    volume *= size(dim);
  return volume;
}

bool Dimensions::contains(const Dimension label) const {
  for (const auto &item : m_dims)
    if (item.first == label)
      return true;
  return false;
}

/// Returns true if all dimensions of other are also contained in *this. Does
/// not check dimension order.
bool Dimensions::contains(const Dimensions &other) const {
  if (*this == other)
    return true;
  for (const auto &item : other.m_dims)
    if (std::find(m_dims.begin(), m_dims.end(), item) == m_dims.end())
      return false;
  return true;
}

Dimension Dimensions::label(const gsl::index i) const {
  return m_dims.at(i).first;
}

gsl::index Dimensions::size(const gsl::index i) const {
  return m_dims.at(i).second;
}

gsl::index Dimensions::size(const Dimension label) const {
  for (const auto &item : m_dims)
    if (item.first == label)
      return item.second;
  throw std::runtime_error("Dimension not found.");
}

/// Return the offset of elements along this dimension in a multi-dimensional
/// array defined by this.
gsl::index Dimensions::offset(const Dimension label) const {
  gsl::index offset{1};
  for (const auto &item : m_dims) {
    if (item.first == label)
      return offset;
    offset *= item.second;
  }
  throw std::runtime_error("Dimension not found.");
}

void Dimensions::resize(const Dimension label, const gsl::index size) {
  if (size <= 0)
    throw std::runtime_error("Dimension size must be positive.");
  for (auto &item : m_dims)
    if (item.first == label) {
      item.second = size;
      return;
    }
  throw std::runtime_error("Dimension not found.");
}

void Dimensions::erase(const Dimension label) {
  m_dims.erase(m_dims.begin() + index(label));
}

void Dimensions::add(const Dimension label, const gsl::index size) {
  // TODO check duplicate dimensions
  m_dims.emplace_back(label, size);
}

gsl::index Dimensions::index(const Dimension label) const {
  for (gsl::index i = 0; i < m_dims.size(); ++i)
    if (m_dims[i].first == label)
      return i;
  throw std::runtime_error("Dimension not found.");
}

Dimensions merge(const Dimensions &a, const Dimensions &b) {
  // TODO order??
  // Not always well defined! Fail if not?!
  auto merged(a);
  for (const auto &item : b) {
    const auto dim = item.first;
    const auto size = item.second;
    if (!a.contains(dim)) {
      merged.add(dim, size);
    } else {
      if (a.size(dim) != size)
        throw std::runtime_error("Size mismatch when merging dimensions.");
    }
  }
  return merged;
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
