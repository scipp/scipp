// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DATASET_NEXT_H
#define DATASET_NEXT_H

#include <optional>
#include <string>
#include <string_view>

#include <boost/iterator/transform_iterator.hpp>

#include "dimension.h"
#include "variable.h"

namespace scipp::core {

namespace next {

class CoordsConstProxy;
class CoordsProxy;
class LabelsConstProxy;
class LabelsProxy;
class Dataset;

namespace detail {
/// Helper for holding data items in Dataset.
struct DatasetData {
  /// Optional data values.
  std::optional<Variable> values;
  /// Optional data variance.
  std::optional<Variable> variances;
  /// Dimension coord for the sparse dimension (there can be only 1).
  std::optional<Variable> coord;
  /// Potential labels for the sparse dimension.
  std::map<std::string, Variable> labels;
};
} // namespace detail

/// Proxy for a data item and related coordinates of Dataset.
class DataConstProxy {
public:
  DataConstProxy(const Dataset &dataset, const detail::DatasetData &data)
      : m_dataset(&dataset), m_data(&data) {}

  bool isSparse() const noexcept;
  Dim sparseDim() const noexcept;
  Dimensions dims() const noexcept;
  scipp::span<const index> shape() const noexcept;
  units::Unit unit() const;

  CoordsConstProxy coords() const noexcept;
  LabelsConstProxy labels() const noexcept;

  /// Return true if the proxy contains data values.
  bool hasValues() const noexcept { return m_data->values.has_value(); }
  /// Return true if the proxy contains data variances.
  bool hasVariances() const noexcept { return m_data->variances.has_value(); }

  /// Return untyped or typed proxy for data values.
  template <class T = void> auto values() const {
    if constexpr (std::is_same_v<T, void>)
      return *m_data->values;
    else
      return m_data->values->span<T>();
  }

  /// Return untyped or typed proxy for data variances.
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

/// Collection of data arrays.
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
  /// Return the number of data items in the dataset.
  ///
  /// This does not include coordinates or attributes, but only all named
  /// entities (which can consist of various combinations of values, variances,
  /// and sparse coordinates).
  index size() const noexcept { return scipp::size(m_data); }
  /// Return true if there are 0 data items in the dataset.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  CoordsConstProxy coords() const noexcept;
  CoordsProxy coords() noexcept;

  LabelsConstProxy labels() const noexcept;
  LabelsProxy labels() noexcept;

  DataConstProxy operator[](const std::string &name) const;

  auto begin() const && = delete;
  /// Return iterator to the beginning of all data items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_data.begin(),
                                          make_const_item{*this});
  }
  auto end() const && = delete;
  /// Return iterator to the end of all data items.
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_data.end(), make_const_item{*this});
  }

  void setCoord(const Dim dim, Variable coord);
  void setLabels(const std::string &labelName, Variable labels);
  void setValues(const std::string &name, Variable values);
  void setVariances(const std::string &name, Variable variances);
  void setSparseCoord(const std::string &name, Variable coord);
  void setSparseLabels(const std::string &name, const std::string &labelName,
                       Variable labels);

private:
  friend class CoordsConstProxy;
  friend class CoordsProxy;
  friend class LabelsConstProxy;
  friend class LabelsProxy;
  friend class DataConstProxy;

  std::map<Dim, Variable> m_coords;
  std::map<std::string, Variable> m_labels;
  std::map<std::string, Variable> m_attrs;
  std::map<std::string, detail::DatasetData> m_data;
};

/// Common functionality for other const-proxy classes.
template <class Key> class ConstProxy {
private:
  static constexpr auto make_const_item = [](const auto &item) {
    return std::pair<Key, ConstVariableSlice>(
        item.first, ConstVariableSlice(*item.second.first));
  };

public:
  using key_type = Key;
  using mapped_type = std::pair<const Variable *, Variable *>;

  ConstProxy(std::map<Key, std::pair<const Variable *, Variable *>> &&items)
      : m_items(std::move(items)) {}

  /// Return the number of coordinates in the proxy.
  index size() const noexcept { return scipp::size(m_items); }
  /// Return true if there are 0 coordinates in the proxy.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  /// Return a const proxy to the coordinate for given dimension.
  ConstVariableSlice operator[](const Key key) const {
    return ConstVariableSlice(*m_items.at(key).first);
  }

  auto begin() const && = delete;
  /// Return const iterator to the beginning of all coordinates.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_items.begin(), make_const_item);
  }
  auto end() const && = delete;
  /// Return const iterator to the end of all coordinates.
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_items.end(), make_const_item);
  }

  const auto &items() const noexcept { return m_items; }

protected:
  std::map<Key, std::pair<const Variable *, Variable *>> m_items;
};

/// Common functionality for other proxy classes.
template <class Derived, class Key> class MutableProxyMixin {
private:
  static constexpr auto make_item = [](const auto &item) {
    return std::pair<Key, VariableSlice>(item.first,
                                         VariableSlice(*item.second.second));
  };

  const Derived &derived() const noexcept {
    return static_cast<const Derived &>(*this);
  }

public:
  /// Return a proxy to the coordinate for given dimension.
  VariableSlice operator[](const Key key) const {
    return VariableSlice(*derived().items().at(key).second);
  }

  auto begin() const && = delete;
  /// Return iterator to the beginning of all coordinates.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(derived().items().begin(), make_item);
  }
  auto end() const && = delete;
  /// Return iterator to the end of all coordinates.
  auto end() const &noexcept {
    return boost::make_transform_iterator(derived().items().end(), make_item);
  }
};

template <class T>
std::pair<const Variable *, Variable *> makeProxyItem(T *variable) {
  if constexpr (std::is_const_v<T>)
    return {variable, nullptr};
  else
    return {variable, variable};
}

template <class T1, class T2>
auto makeCoordsProxyItems(T1 &coords, const Dim sparseDim, T2 &sparseCoord) {
  std::map<Dim, std::pair<const Variable *, Variable *>> items;
  if (sparseDim == Dim::Invalid) {
    for (auto &item : coords)
      items.emplace(item.first, makeProxyItem(&item.second));
  } else {
    // Shadow all global coordinates that depend on the sparse dimension.
    for (auto &item : coords)
      if (!item.second.dimensions().contains(sparseDim))
        items.emplace(item.first, makeProxyItem(&item.second));
    if (sparseCoord)
      items.emplace(sparseDim, makeProxyItem(&*sparseCoord));
  }
  return items;
}

template <class T1, class T2>
auto makeLabelsProxyItems(T1 &labels, const Dim sparseDim, T2 *sparseLabels) {
  std::map<std::string_view, std::pair<const Variable *, Variable *>> items;
  if (sparseDim == Dim::Invalid) {
    for (const auto &item : labels)
      items.emplace(item.first, makeProxyItem(&item.second));
  } else {
    // Shadow all global labels that depend on the sparse dimension.
    for (const auto &item : labels)
      if (!item.second.dimensions().contains(sparseDim))
        items.emplace(item.first, makeProxyItem(&item.second));
    if (sparseLabels)
      for (const auto &item : *sparseLabels)
        items.emplace(item.first, makeProxyItem(&item.second));
  }
  return items;
}

/// Proxy for accessing coordinates of const Dataset and DataConstProxy.
class CoordsConstProxy : public ConstProxy<Dim> {
public:
  using ConstProxy<Dim>::ConstProxy;
  explicit CoordsConstProxy(
      const Dataset &dataset, const Dim sparseDim = Dim::Invalid,
      const std::optional<Variable> &sparseCoord = std::nullopt)
      : ConstProxy<Dim>(
            makeCoordsProxyItems(dataset.m_coords, sparseDim, sparseCoord)) {}
};

/// Proxy for accessing coordinates of Dataset and DataProxy.
class CoordsProxy
    : public MutableProxyMixin<CoordsProxy, CoordsConstProxy::key_type>,
      public CoordsConstProxy {
public:
  explicit CoordsProxy(Dataset &dataset, const Dim sparseDim = Dim::Invalid,
                       Variable *sparseCoord = nullptr)
      : CoordsConstProxy(
            makeCoordsProxyItems(dataset.m_coords, sparseDim, sparseCoord)) {}
  using MutableProxyMixin<CoordsProxy, CoordsConstProxy::key_type>::operator[];
  using MutableProxyMixin<CoordsProxy, CoordsConstProxy::key_type>::begin;
  using MutableProxyMixin<CoordsProxy, CoordsConstProxy::key_type>::end;
};

/// Proxy for accessing labels of const Dataset and DataConstProxy.
class LabelsConstProxy : public ConstProxy<std::string_view> {
public:
  using ConstProxy<std::string_view>::ConstProxy;
  explicit LabelsConstProxy(
      const Dataset &dataset, const Dim sparseDim = Dim::Invalid,
      const std::map<std::string, Variable> *sparseLabels = nullptr)
      : ConstProxy<std::string_view>(
            makeLabelsProxyItems(dataset.m_labels, sparseDim, sparseLabels)) {}
};

/// Proxy for accessing labels of Dataset and DataProxy.
class LabelsProxy
    : public MutableProxyMixin<LabelsProxy, LabelsConstProxy::key_type>,
      public LabelsConstProxy {
public:
  explicit LabelsProxy(Dataset &dataset, const Dim sparseDim = Dim::Invalid,
                       std::map<std::string, Variable> *sparseLabels = nullptr)
      : LabelsConstProxy(
            makeLabelsProxyItems(dataset.m_labels, sparseDim, sparseLabels)) {}
  using MutableProxyMixin<LabelsProxy, LabelsConstProxy::key_type>::operator[];
  using MutableProxyMixin<LabelsProxy, LabelsConstProxy::key_type>::begin;
  using MutableProxyMixin<LabelsProxy, LabelsConstProxy::key_type>::end;
};

} // namespace next
} // namespace scipp::core

#endif // DATASET_NEXT_H
