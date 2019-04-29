// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DATASET_NEXT_H
#define DATASET_NEXT_H

#include <optional>

#include <boost/iterator/transform_iterator.hpp>

#include "dimension.h"
#include "variable.h"

namespace scipp::core {

namespace next {

class CoordsConstProxy;
class Dataset;

namespace detail {
struct DatasetData {
  std::optional<Variable> values;
  std::optional<Variable> variances;
  /// Dimension coord for sparse dim (there can be only 1):
  std::optional<Variable> coord;
  std::map<std::string, Variable> labels;
};
} // namespace detail

class DataConstProxy {
public:
  DataConstProxy(const Dataset &dataset, const detail::DatasetData &data)
      : m_dataset(&dataset), m_data(&data) {}

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
  const detail::DatasetData *m_data;
};

class Dataset {
private:
  struct make_const_item {
    const Dataset &dataset;
    std::pair<const std::string &, DataConstProxy>
    operator()(const auto &item) const {
      return std::pair(item.first, DataConstProxy(dataset, item.second));
    }
  };

public:
  index size() const noexcept { return scipp::size(m_data); }
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  CoordsConstProxy coords() const noexcept;

  DataConstProxy operator[](const std::string &name) const;

  auto begin() const && = delete;
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_data.begin(),
                                          make_const_item{*this});
  }
  auto end() const && = delete;
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_data.end(), make_const_item{*this});
  }

  void setCoord(const Dim dim, Variable coord);
  void setValues(const std::string &name, Variable values);
  void setVariances(const std::string &name, Variable variances);
  void setSparseCoord(const std::string &name, Variable coord);

private:
  friend class CoordsConstProxy;
  friend class DataConstProxy;

  std::map<Dim, Variable> m_coords;
  std::map<std::string, Variable> m_labels;
  std::map<std::string, detail::DatasetData> m_data;
};

class CoordsConstProxy {
private:
  static constexpr auto make_const_item = [](const auto &item) {
    return std::pair(item.first, ConstVariableSlice(*item.second));
  };

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

  index size() const noexcept { return scipp::size(m_coords); }
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  ConstVariableSlice operator[](const Dim dim) const;

  auto begin() const && = delete;
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_coords.begin(), make_const_item);
  }
  auto end() const && = delete;
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_coords.end(), make_const_item);
  }

private:
  std::map<Dim, const Variable *> m_coords;
};

} // namespace next
} // namespace scipp::core

#endif // DATASET_NEXT_H
