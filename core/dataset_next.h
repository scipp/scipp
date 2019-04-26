// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DATASET_NEXT_H
#define DATASET_NEXT_H

#include <optional>
#include <vector>

#include <boost/container/small_vector.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include "dimension.h"
#include "variable.h"

namespace scipp::core {

namespace next {

class CoordsConstProxy;
class DataConstProxy;

class Dataset {
public:
  index size() const noexcept { return scipp::size(m_data); }

  // ConstVariableSlice coords(const Dim dim) const;
  // VariableSlice coords(const Dim dim);

  CoordsConstProxy coords() const noexcept;
  CoordsConstProxy labels() const noexcept;
  // CoordsProxy coords();

  DataConstProxy operator[](const std::string &name) const;
  // DataProxy operator[](const std::string &name);

private:
  friend class CoordsConstProxy;
  class DataCols {
    std::optional<Variable> m_values;
    std::optional<Variable> m_variances;
    /// Dimension coord for sparse dim (there can be only 1):
    std::optional<Variable> m_coord;
    std::map<std::string, Variable> m_labels;
  };

  std::map<Dim, Variable> m_coords;
  std::map<std::string, Variable> m_labels;
  std::map<std::string, DataCols> m_data;
};

class CoordsConstProxy {
public:
  explicit CoordsConstProxy(const Dataset *dataset,
                            const std::string *name = nullptr)
      : m_dataset(dataset), m_name(name) {}

  ConstVariableSlice operator[](const Dim dim) const;

  index size() const noexcept {
    if (!m_name)
      return scipp::size(m_dataset->m_coords);
    // If data is sparse we must hide global coords, even if there is no
    // name-specific coord.
    // TODO Sparse data without coords does not make sense, can we check this
    // somewhere at creation time?
    // TODO If the coord exists it should imply that the data is sparse? In
    // principle we can imagine supporting a more general unaligned type of
    // data, even if it is not sparse?
    if (!m_data[*m_name].m_coord)
      return scipp::size(m_dataset->m_coords);
    // Count all shadowed global coordinates.
    const auto dim = *m_data[*m_name].m_coord;
    index count;
    for (const auto &coord : m_dataset->m_coords)
      if (coord.dimensions().contains(dim))
        ++count;
    // +1 is the local (sparse) coordinate.
    return scipp::size(m_dataset->m_coords) + 1 - count;
  }

private:
  const Dataset *m_dataset;
  const std::string *m_name;
};

class LabelsConstProxy {
public:
  explicit LabelsConstProxy(const Dataset *dataset) : m_dataset(dataset) {}

  ConstVariableSlice operator[](const std::string &name) const;

  index size() const noexcept { return scipp::size(m_dataset->m_labels); }

private:
  const Dataset *m_dataset;
};

class DataConstProxy {
public:
  DataConstProxy(const Dataset *dataset, const std::string &name)
      : m_dataset(dataset), m_name(name) {}

  // would need more complicated implementation, coords in different sections
  CoordsConstProxy coords() const noexcept;

  // should we provide this, or just `values<T>()`?
  // ConstVariableSlice values() const;
  // ConstVariableSlice variances() const;

  // Returns a typed view (VariableView<T> or span<T>)?
  template <class T = void> auto values() const;
  template <class T = void> auto variances() const;

  // Unit unit() const;
  // void setUnit(const Unit unit);

private:
  const Dataset *m_dataset;
  const std::string &m_name;
};

} // namespace next
} // namespace scipp::core

#endif // DATASET_NEXT_H
