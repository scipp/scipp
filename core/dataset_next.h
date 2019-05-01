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

class Dataset;

namespace ProxyId {
class Coords;
class Labels;
}
template <class Id, class Key> class ConstProxy;
template <class Base> class MutableProxy;

/// Proxy for accessing coordinates of const Dataset and DataConstProxy.
using CoordsConstProxy = ConstProxy<ProxyId::Coords, Dim>;
/// Proxy for accessing coordinates of Dataset and DataProxy.
using CoordsProxy = MutableProxy<CoordsConstProxy>;
/// Proxy for accessing labels of const Dataset and DataConstProxy.
using LabelsConstProxy = ConstProxy<ProxyId::Labels, std::string_view>;
/// Proxy for accessing labels of Dataset and DataProxy.
using LabelsProxy = MutableProxy<LabelsConstProxy>;

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

/// Const proxy for a data item and related coordinates of Dataset.
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

  /// Return untyped or typed const proxy for data values.
  template <class T = void> auto values() const {
    if constexpr (std::is_same_v<T, void>)
      return *m_data->values;
    else
      return m_data->values->span<T>();
  }

  /// Return untyped or typed const proxy for data variances.
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

/// Proxy for a data item and related coordinates of Dataset.
class DataProxy : public DataConstProxy {
public:
  DataProxy(Dataset &dataset, detail::DatasetData &data)
      : DataConstProxy(dataset, data), m_mutableDataset(&dataset),
        m_mutableData(&data) {}

  CoordsProxy coords() const noexcept;
  LabelsProxy labels() const noexcept;

  /// Return untyped or typed proxy for data values.
  template <class T = void> auto values() const {
    if constexpr (std::is_same_v<T, void>)
      return *m_mutableData->values;
    else
      return m_mutableData->values->span<T>();
  }

  /// Return untyped or typed proxy for data variances.
  template <class T = void> auto variances() const {
    if constexpr (std::is_same_v<T, void>)
      return *m_mutableData->variances;
    else
      return m_mutableData->variances->span<T>();
  }

private:
  Dataset *m_mutableDataset;
  detail::DatasetData *m_mutableData;
};

namespace detail {
/// Helper for creating iterators of Dataset.
template <class D> struct make_item {
  D *dataset;
  using P = std::conditional_t<std::is_const_v<D>, DataConstProxy, DataProxy>;
  std::pair<std::string_view, P> operator()(auto &item) const {
    return {item.first, P(*dataset, item.second)};
  }
};
template <class D> make_item(D *)->make_item<D>;
} // namespace detail

class DatasetConstSlice;
class DatasetSlice;

/// Collection of data arrays.
class Dataset {
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
  DataProxy operator[](const std::string &name);

  auto begin() const && = delete;
  auto begin() && = delete;
  /// Return const iterator to the beginning of all data items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_data.begin(),
                                          detail::make_item{this});
  }
  /// Return iterator to the beginning of all data items.
  auto begin() & noexcept {
    return boost::make_transform_iterator(m_data.begin(),
                                          detail::make_item{this});
  }
  auto end() const && = delete;
  auto end() && = delete;
  /// Return const iterator to the end of all data items.
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_data.end(),
                                          detail::make_item{this});
  }
  /// Return iterator to the end of all data items.
  auto end() & noexcept {
    return boost::make_transform_iterator(m_data.end(),
                                          detail::make_item{this});
  }

  void setCoord(const Dim dim, Variable coord);
  void setLabels(const std::string &labelName, Variable labels);
  void setValues(const std::string &name, Variable values);
  void setVariances(const std::string &name, Variable variances);
  void setSparseCoord(const std::string &name, Variable coord);
  void setSparseLabels(const std::string &name, const std::string &labelName,
                       Variable labels);

  struct Slice {
    Slice(const Dim dim, const scipp::index begin, const scipp::index end = -1)
        : dim(dim), begin(begin), end(end) {}
    Dim dim;
    scipp::index begin;
    scipp::index end;
  };

  DatasetConstSlice slice(const Dim dim, const scipp::index begin,
                          const scipp::index end = -1) const;
  DatasetConstSlice slice(const Slice slice) const;
  DatasetConstSlice slice(const Slice slice1, const Slice slice2) const;

private:
  friend class DataConstProxy;
  friend class DataProxy;

  std::map<Dim, Variable> m_coords;
  std::map<std::string, Variable> m_labels;
  std::map<std::string, Variable> m_attrs;
  std::map<std::string, detail::DatasetData> m_data;
};

namespace detail {
template <class VarSlice>
auto makeSlice(VarSlice slice, const std::vector<Dataset::Slice> &slices) {
  for (const auto[dim, begin, end] : slices)
    if (slice.dimensions().contains(dim))
      slice = slice(dim, begin, end);
  return slice;
}
} // namespace detail

/// Common functionality for other const-proxy classes.
template <class Id, class Key> class ConstProxy {
private:
  struct make_item {
    const ConstProxy *proxy;
    auto operator()(const auto &item) const {
      return std::pair<Key, ConstVariableSlice>(
          item.first, detail::makeSlice(ConstVariableSlice(*item.second.first),
                                        proxy->slices()));
    }
  };

public:
  using key_type = Key;

  ConstProxy(std::map<Key, std::pair<const Variable *, Variable *>> &&items)
      : m_items(std::move(items)) {}

  /// Return the number of coordinates in the proxy.
  index size() const noexcept { return scipp::size(m_items); }
  /// Return true if there are 0 coordinates in the proxy.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  /// Return a const proxy to the coordinate for given dimension.
  ConstVariableSlice operator[](const Key key) const {
    return detail::makeSlice(ConstVariableSlice(*m_items.at(key).first),
                             m_slices);
  }

  auto begin() const && = delete;
  /// Return const iterator to the beginning of all items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_items.begin(), make_item{this});
  }
  auto end() const && = delete;
  /// Return const iterator to the end of all items.
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_items.end(), make_item{this});
  }

  ConstProxy slice(const Dataset::Slice slice) const {
    std::map<Key, std::pair<const Variable *, Variable *>> items;
    std::copy_if(m_items.begin(), m_items.end(),
                 std::inserter(items, items.end()), [slice](const auto &item) {
                   // Delete coords that do not depend in dim, and coord of
                   // sliced dimension if slice is not a range.
                   return item.second.first->dimensions().contains(slice.dim) &&
                          !((slice.end == -1) && (item.first == slice.dim));
                 });
    ConstProxy sliced(std::move(items));
    sliced.m_slices = m_slices;
    sliced.m_slices.push_back(slice);
    return sliced;
  }

  ConstProxy slice(const Dataset::Slice slice1,
                   const Dataset::Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  ConstProxy slice(const Dataset::Slice slice1, const Dataset::Slice slice2,
                   const Dataset::Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  const auto &items() const noexcept { return m_items; }
  const auto &slices() const noexcept { return m_slices; }

protected:
  std::map<Key, std::pair<const Variable *, Variable *>> m_items;
  std::vector<Dataset::Slice> m_slices;
};

/// Common functionality for other proxy classes.
template <class Base> class MutableProxy : public Base {
private:
  struct make_item {
    const MutableProxy<Base> *proxy;
    auto operator()(const auto &item) const {
      return std::pair<typename Base::key_type, VariableSlice>(
          item.first, detail::makeSlice(VariableSlice(*item.second.second),
                                        proxy->slices()));
    }
  };

  explicit MutableProxy(Base &&base) : Base(std::move(base)) {}

public:
  using Base::Base;

  /// Return a proxy to the coordinate for given dimension.
  VariableSlice operator[](const typename Base::key_type key) const {
    return detail::makeSlice(VariableSlice(*Base::items().at(key).second),
                             Base::slices());
  }

  auto begin() const && = delete;
  /// Return iterator to the beginning of all items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(Base::items().begin(),
                                          make_item{this});
  }
  auto end() const && = delete;
  /// Return iterator to the end of all items.
  auto end() const &noexcept {
    return boost::make_transform_iterator(Base::items().end(), make_item{this});
  }

  MutableProxy slice(const Dataset::Slice slice) const {
    return MutableProxy(Base::slice(slice));
  }

  MutableProxy slice(const Dataset::Slice slice1,
                     const Dataset::Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  MutableProxy slice(const Dataset::Slice slice1, const Dataset::Slice slice2,
                     const Dataset::Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }
};

/*
class DatasetConstSlice {
public:
  DatasetConstSlice(const Dataset &dataset,
                    const std::initializer_list<Dataset::Slice> &slices)
      : m_dataset(&dataset), m_slices(slices) {}

  index size() const noexcept { return scipp::size(m_data); }
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  CoordsConstProxy coords() const noexcept;

  LabelsConstProxy labels() const noexcept;

  DataConstProxy operator[](const std::string &name) const;

  auto begin() const && = delete;
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_data.begin(),
                                          detail::make_item{this});
  }
  auto end() const && = delete;
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_data.end(),
                                          detail::make_item{this});
  }

private:
  const Dataset *m_dataset;
  std::vector<Dataset::Slice> m_slices;
};
*/

} // namespace next
} // namespace scipp::core

#endif // DATASET_NEXT_H
