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
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  // ConstVariableSlice coords(const Dim dim) const;
  // VariableSlice coords(const Dim dim);

  CoordsConstProxy coords() const noexcept;
  CoordsConstProxy labels() const noexcept;
  // CoordsProxy coords();

  DataConstProxy operator[](const std::string &name) const;
  // DataProxy operator[](const std::string &name);

  void setCoord(const Dim dim, Variable coord);
  void setValues(const std::string &name, Variable values);
  void setVariances(const std::string &name, Variable variances);
  // void setCoord(const std::string &name, const Dim dim, Variable values);

private:
  friend class CoordsConstProxy;
  friend class LabelsConstProxy;
  friend class DataConstProxy;

  struct DataCols {
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
  explicit CoordsConstProxy(const Dataset &dataset,
                            const Dim sparseDim = Dim::Invalid,
                            const Variable *sparseCoord = nullptr) {
    if (sparseDim == Dim::Invalid) {
      for (const auto &item : dataset.m_coords)
        m_coords.emplace(item.first, &item.second);
    } else {
      // Shadow all global coordinates that depend on the sparse dimension.
      for (const auto &item : dataset.m_coords)
        if (!item.second.dimensions().contains(sparseDim))
          m_coords.emplace(item.first, &item.second);
      if (sparseCoord)
        m_coords.emplace(sparseDim, sparseCoord);
    }
  }

  ConstVariableSlice operator[](const Dim dim) const;

  index size() const noexcept { return scipp::size(m_coords); }

private:
  std::map<Dim, const Variable *> m_coords;
};

/*
class LabelsConstProxy {
public:
  explicit LabelsConstProxy(const Dataset *dataset) : m_dataset(dataset) {}

  ConstVariableSlice operator[](const std::string &name) const;

  index size() const noexcept { return scipp::size(m_dataset->m_labels); }

private:
  const Dataset *m_dataset;
}*/

class DataConstProxy {
public:
  DataConstProxy(const Dataset *dataset, const std::string &name)
      : m_dataset(dataset), m_name(&name) {}

  // would need more complicated implementation, coords in different sections
  CoordsConstProxy coords() const noexcept;

  bool hasValues() const noexcept {
    return m_dataset->m_data.at(*m_name).m_values.has_value();
  }
  bool hasVariances() const noexcept {
    return m_dataset->m_data.at(*m_name).m_variances.has_value();
  }

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
  const std::string *m_name;
};

} // namespace next
} // namespace scipp::core

#endif // DATASET_NEXT_H
