// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DATASET_NEXT_H
#define DATASET_NEXT_H

#include <optional>

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

  CoordsConstProxy coords() const noexcept;

  DataConstProxy operator[](const std::string &name) const;

  void setCoord(const Dim dim, Variable coord);
  void setValues(const std::string &name, Variable values);
  void setVariances(const std::string &name, Variable variances);
  void setSparseCoord(const std::string &name, Variable coord);

private:
  friend class CoordsConstProxy;
  friend class DataConstProxy;

  struct Data {
    std::optional<Variable> values;
    std::optional<Variable> variances;
    /// Dimension coord for sparse dim (there can be only 1):
    std::optional<Variable> coord;
    std::map<std::string, Variable> labels;
  };

  std::map<Dim, Variable> m_coords;
  std::map<std::string, Variable> m_labels;
  std::map<std::string, Data> m_data;
};

class CoordsConstProxy {
public:
  explicit CoordsConstProxy(
      const Dataset &dataset, const Dim sparseDim = Dim::Invalid,
      const std::optional<Variable> &sparseCoord = std::nullopt) {
    if (sparseDim == Dim::Invalid) {
      for (const auto &item : dataset.m_coords)
        m_coords.emplace(item.first, &item.second);
    } else {
      // Shadow all global coordinates that depend on the sparse dimension.
      for (const auto &item : dataset.m_coords)
        if (!item.second.dimensions().contains(sparseDim))
          m_coords.emplace(item.first, &item.second);
      if (sparseCoord)
        m_coords.emplace(sparseDim, &*sparseCoord);
    }
  }

  ConstVariableSlice operator[](const Dim dim) const;

  index size() const noexcept { return scipp::size(m_coords); }

private:
  std::map<Dim, const Variable *> m_coords;
};

class DataConstProxy {
public:
  DataConstProxy(const Dataset *dataset, const std::string &name)
      : m_dataset(dataset), m_data(&m_dataset->m_data.at(name)) {}

  bool isSparse() const noexcept;
  Dim sparseDim() const noexcept;
  scipp::span<const Dim> dims() const noexcept;
  scipp::span<const index> shape() const noexcept;
  units::Unit unit() const;

  CoordsConstProxy coords() const noexcept;

  bool hasValues() const noexcept { return m_data->values.has_value(); }
  bool hasVariances() const noexcept { return m_data->variances.has_value(); }

  template <class T = void> auto values() const {
    if constexpr (std::is_same_v<T, void>)
      return *m_data->values;
    else
      return m_data->values->span<T>();
  }

  template <class T = void> auto variances() const {
    if constexpr (std::is_same_v<T, void>)
      return *m_data->variances;
    else
      return m_data->variances->span<T>();
  }

private:
  const Dataset *m_dataset;
  const Dataset::Data *m_data;
};

} // namespace next
} // namespace scipp::core

#endif // DATASET_NEXT_H
