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
Dimensions::Dimensions(const Dimensions &other) : m_dims(other.m_dims) {
  if (other.m_raggedDim)
    m_raggedDim = std::make_unique<Variable>(*other.m_raggedDim);
}
Dimensions::Dimensions(Dimensions &&other) = default;
Dimensions::~Dimensions() = default;
Dimensions &Dimensions::operator=(const Dimensions &other) {
  auto copy(other);
  std::swap(*this, copy);
}
Dimensions &Dimensions::operator=(Dimensions &&other) = default;

bool Dimensions::operator==(const Dimensions &other) const {
  // Ragged comparison too complex for now.
  if (m_raggedDim || other.m_raggedDim)
    return false;
  return m_dims == other.m_dims;
}

bool Dimensions::isRagged() const { return m_raggedDim != nullptr; }

gsl::index Dimensions::count() const { return m_dims.size(); }

gsl::index Dimensions::volume() const {
  gsl::index volume{1};
  gsl::index raggedCorrection{1};
  for (gsl::index dim = 0; dim < count(); ++dim) {
    if (isRagged(dim)) {
      const auto &raggedInfo = raggedSize(dim);
      const auto &dependentDimensions = raggedInfo.dimensions();
      gsl::index found{0};
      for (gsl::index dim2 = 0; dim2 < dependentDimensions.count(); ++dim2) {
        for (gsl::index dim3 = dim + 1; dim3 < count(); ++dim3) {
          if (dependentDimensions.label(dim2) == label(dim3)) {
            ++found;
            break;
          }
        }
      }
      if (found != dependentDimensions.count())
        throw std::runtime_error(
            "Ragged size information contains extra dimensions.");
      const auto &sizes = raggedInfo.get<const Data::DimensionSize>();
      volume *= std::accumulate(sizes.begin(), sizes.end(), 0);
      raggedCorrection = dependentDimensions.volume();
    } else {
      volume *= size(dim);
    }
  }
  volume /= raggedCorrection;
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
  // Ragged comparison too complex for now.
  if (m_raggedDim || other.m_raggedDim)
    return false;
  for (const auto &item : other.m_dims)
    if (std::find(m_dims.begin(), m_dims.end(), item) == m_dims.end())
      return false;
  return true;
}

bool Dimensions::isRagged(const gsl::index i) const {
  const auto size = m_dims.at(i).second;
  return size == -1;
}

bool Dimensions::isRagged(const Dimension label) const {
  return isRagged(index(label));
}

Dimension Dimensions::label(const gsl::index i) const {
  return m_dims.at(i).first;
}

gsl::index Dimensions::size(const gsl::index i) const {
  const auto size = m_dims.at(i).second;
  if (size == -1)
    throw std::runtime_error(
        "Dimension is ragged, size() not available, use raggedSize().");
  return size;
}

gsl::index Dimensions::size(const Dimension label) const {
  for (const auto &item : m_dims)
    if (item.first == label) {
      if (item.second == -1)
        throw std::runtime_error(
            "Dimension is ragged, size() not available, use raggedSize().");
      return item.second;
    }
  throw std::runtime_error("Dimension not found.");
}

/// Return the offset of elements along this dimension in a multi-dimensional
/// array defined by this.
gsl::index Dimensions::offset(const Dimension label) const {
  gsl::index offset{1};
  for (const auto &item : m_dims) {
    if (item.second == -1)
      throw std::runtime_error("Dimension is ragged, offset() not available.");
    if (item.first == label) {
      return offset;
    }
    offset *= item.second;
  }
  throw std::runtime_error("Dimension not found.");
}

void Dimensions::resize(const Dimension label, const gsl::index size) {
  if (size <= 0)
    throw std::runtime_error("Dimension size must be positive.");
  for (auto &item : m_dims)
    if (item.first == label) {
      if (item.second == -1)
        throw std::runtime_error(
            "Dimension is ragged, resize() not available, use resizeRagged().");
      item.second = size;
      return;
    }
  throw std::runtime_error("Dimension not found.");
}

void Dimensions::erase(const Dimension label) {
  if (m_raggedDim)
    throw std::runtime_error(
        "Dimensions::erase not implemented if any dimension is ragged.");

  m_dims.erase(m_dims.begin() + index(label));
}

const Variable &Dimensions::raggedSize(const gsl::index i) const {
  if (m_dims.at(i).second != -1)
    throw std::runtime_error(
        "Dimension is not ragged, use size() instead of raggedSize().");
  return *m_raggedDim;
}

const Variable &Dimensions::raggedSize(const Dimension label) const {
  return raggedSize(index(label));
}

void Dimensions::add(const Dimension label, const gsl::index size) {
  // TODO check duplicate dimensions
  m_dims.emplace_back(label, size);
}

void Dimensions::add(const Dimension label, const Variable &raggedSize) {
  // TODO check duplicate dimensions
  if (!raggedSize.valueTypeIs<Data::DimensionSize>())
    throw std::runtime_error("Variable with sizes information for ragged "
                             "dimension is of wrong type.");
  m_dims.emplace_back(label, -1);
  if (m_raggedDim)
    throw std::runtime_error("Only one dimension can be ragged.");
  m_raggedDim = std::make_unique<Variable>(raggedSize);
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
      if (size == -1)
        merged.add(dim, b.raggedSize(dim));
      else
        merged.add(dim, size);
    } else {
      if (a.isRagged(dim)) {
        if (size == -1) {
          if (a.raggedSize(dim).get<const Data::DimensionSize>() !=
              b.raggedSize(dim).get<const Data::DimensionSize>())
            throw std::runtime_error("Size mismatch when merging dimensions.");
        } else {
          throw std::runtime_error("Size mismatch when merging dimensions.");
        }
      } else {
        if (a.size(dim) != size)
          throw std::runtime_error("Size mismatch when merging dimensions.");
      }
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
