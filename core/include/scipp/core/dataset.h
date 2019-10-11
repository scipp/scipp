// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_DATASET_H
#define SCIPP_DATASET_H

#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <unordered_map>

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/except.h"
#include "scipp/core/slice.h"
#include "scipp/core/variable.h"

namespace scipp::core {

class DataArray;
class Dataset;
class DatasetConstProxy;
class DatasetProxy;

namespace ProxyId {
class Attrs;
class Coords;
class Labels;
} // namespace ProxyId
template <class Id, class Key> class ConstProxy;
template <class Base> class MutableProxy;

/// Proxy for accessing coordinates of const Dataset and DataConstProxy.
using CoordsConstProxy = ConstProxy<ProxyId::Coords, Dim>;
/// Proxy for accessing coordinates of Dataset and DataProxy.
using CoordsProxy = MutableProxy<CoordsConstProxy>;
/// Proxy for accessing labels of const Dataset and DataConstProxy.
using LabelsConstProxy = ConstProxy<ProxyId::Labels, std::string>;
/// Proxy for accessing labels of Dataset and DataProxy.
using LabelsProxy = MutableProxy<LabelsConstProxy>;
/// Proxy for accessing attributes of const Dataset and DataConstProxy.
using AttrsConstProxy = ConstProxy<ProxyId::Attrs, std::string>;
/// Proxy for accessing attributes of Dataset and DataProxy.
using AttrsProxy = MutableProxy<AttrsConstProxy>;

namespace detail {
/// Helper for holding data items in Dataset.
struct DatasetData {
  /// Optional data values (with optional variances).
  std::optional<Variable> data;
  /// Dimension coord for the sparse dimension (there can be only 1).
  std::optional<Variable> coord;
  /// Potential labels for the sparse dimension.
  std::unordered_map<std::string, Variable> labels;
};

using dataset_item_map = std::unordered_map<std::string, DatasetData>;

template <class Var>
auto makeSlice(Var &var,
               const std::vector<std::pair<Slice, scipp::index>> &slices) {
  std::conditional_t<std::is_const_v<Var>, VariableConstProxy, VariableProxy>
      slice(var);
  for (const auto[params, extent] : slices) {
    if (slice.dims().contains(params.dim())) {
      const auto new_end = params.end() + slice.dims()[params.dim()] - extent;
      const auto pointSlice = (new_end == -1);
      if (pointSlice) {
        slice = slice.slice(Slice{params.dim(), params.begin()});
      } else {
        slice = slice.slice(Slice{params.dim(), params.begin(), new_end});
      }
    }
  }
  return slice;
}
} // namespace detail

/// Const proxy for a data item and related coordinates of Dataset.
class SCIPP_CORE_EXPORT DataConstProxy {
public:
  DataConstProxy(const Dataset &dataset,
                 const detail::dataset_item_map::value_type &data,
                 const std::vector<std::pair<Slice, scipp::index>> &slices = {})
      : m_dataset(&dataset), m_data(&data), m_slices(slices) {}

  const std::string &name() const noexcept;

  Dimensions dims() const noexcept;
  DType dtype() const;
  units::Unit unit() const;

  CoordsConstProxy coords() const noexcept;
  LabelsConstProxy labels() const noexcept;
  AttrsConstProxy attrs() const noexcept;

  /// Return true if the proxy contains data values.
  bool hasData() const noexcept { return m_data->second.data.has_value(); }
  /// Return true if the proxy contains data variances.
  bool hasVariances() const noexcept {
    return hasData() && m_data->second.data->hasVariances();
  }

  /// Return untyped const proxy for data (values and optional variances).
  VariableConstProxy data() const {
    if (!hasData())
      throw except::SparseDataError("No data in item.");
    return detail::makeSlice(*m_data->second.data, slices());
  }
  /// Return typed const proxy for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed const proxy for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  DataConstProxy slice(const Slice slice1) const {
    expect::validSlice(dims(), slice1);
    auto tmp(m_slices);
    tmp.emplace_back(slice1, dims()[slice1.dim()]);
    return {*m_dataset, *m_data, std::move(tmp)};
  }

  DataConstProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  DataConstProxy slice(const Slice slice1, const Slice slice2,
                       const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  const std::vector<std::pair<Slice, scipp::index>> &slices() const noexcept {
    return m_slices;
  }

  auto &underlying() const { return m_data->second; }

private:
  friend class DatasetConstProxy;
  friend class DatasetProxy;

  const Dataset *m_dataset;
  const detail::dataset_item_map::value_type *m_data;
  std::vector<std::pair<Slice, scipp::index>> m_slices;
};

SCIPP_CORE_EXPORT bool operator==(const DataConstProxy &a,
                                  const DataConstProxy &b);
SCIPP_CORE_EXPORT bool operator!=(const DataConstProxy &a,
                                  const DataConstProxy &b);

/// Proxy for a data item and related coordinates of Dataset.
class SCIPP_CORE_EXPORT DataProxy : public DataConstProxy {
public:
  DataProxy(Dataset &dataset, detail::dataset_item_map::value_type &data,
            const std::vector<std::pair<Slice, scipp::index>> &slices = {})
      : DataConstProxy(dataset, data, slices), m_mutableDataset(&dataset),
        m_mutableData(&data) {}

  CoordsProxy coords() const noexcept;
  LabelsProxy labels() const noexcept;
  AttrsProxy attrs() const noexcept;

  void setUnit(const units::Unit unit) const;

  /// Set variances for the data. If the data has no variances array, it is
  /// created.
  template <class T> void setVariances(Vector<T> &&v) const {
    data().setVariances(std::move(v));
  }

  /// Return untyped proxy for data (values and optional variances).
  VariableProxy data() const {
    if (!hasData())
      throw except::SparseDataError("No data in item.");
    return detail::makeSlice(*m_mutableData->second.data, slices());
  }
  /// Return typed proxy for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed proxy for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  DataProxy slice(const Slice slice1) const {
    expect::validSlice(dims(), slice1);
    auto tmp(slices());
    tmp.emplace_back(slice1, dims()[slice1.dim()]);
    return {*m_mutableDataset, *m_mutableData, std::move(tmp)};
  }

  DataProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  DataProxy slice(const Slice slice1, const Slice slice2,
                  const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  DataProxy assign(const DataConstProxy &other) const;
  DataProxy assign(const Variable &other) const;
  DataProxy assign(const VariableConstProxy &other) const;
  DataProxy operator+=(const DataConstProxy &other) const;
  DataProxy operator-=(const DataConstProxy &other) const;
  DataProxy operator*=(const DataConstProxy &other) const;
  DataProxy operator/=(const DataConstProxy &other) const;
  DataProxy operator+=(const VariableConstProxy &other) const;
  DataProxy operator-=(const VariableConstProxy &other) const;
  DataProxy operator*=(const VariableConstProxy &other) const;
  DataProxy operator/=(const VariableConstProxy &other) const;

private:
  Dataset *m_mutableDataset;
  detail::dataset_item_map::value_type *m_mutableData;
};

namespace detail {
template <class T> struct is_const;
template <> struct is_const<Dataset> : std::false_type {};
template <> struct is_const<const Dataset> : std::true_type {};
template <> struct is_const<const DatasetProxy> : std::false_type {};
template <> struct is_const<const DatasetConstProxy> : std::true_type {};

/// Helper for creating iterators of Dataset.
template <class D> struct make_item {
  D *dataset;
  using P = std::conditional_t<is_const<D>::value, DataConstProxy, DataProxy>;
  template <class T>
  std::pair<const std::string &, P> operator()(T &item) const {
    if constexpr (std::is_same_v<std::remove_const_t<D>, Dataset>)
      return {item.first, P(*dataset, item)};
    else
      // TODO Using operator[] is quite inefficient, revert the logic.
      return {item, dataset->operator[](item)};
  }
};
template <class D> make_item(D *)->make_item<D>;
} // namespace detail

class DatasetConstProxy;
class DatasetProxy;

/// Collection of data arrays.
class SCIPP_CORE_EXPORT Dataset {
public:
  using key_type = std::string;
  using mapped_type = DataArray;
  using value_type = std::pair<const std::string &, DataConstProxy>;

  Dataset() = default;
  explicit Dataset(const DatasetConstProxy &proxy);
  explicit Dataset(const DataConstProxy &data);
  explicit Dataset(const std::map<std::string, DataConstProxy> &data);

  template <class DataMap, class CoordMap, class LabelsMap, class AttrMap>
  Dataset(DataMap data, CoordMap coords, LabelsMap labels, AttrMap attrs) {
    for (auto && [ dim, coord ] : coords)
      setCoord(dim, std::move(coord));
    for (auto && [ name, labs ] : labels)
      setLabels(std::string(name), std::move(labs));
    for (auto && [ name, attr ] : attrs)
      setAttr(std::string(name), std::move(attr));
    for (auto && [ name, item ] : data)
      setData(std::string(name), std::move(item));
  }

  /// Return the number of data items in the dataset.
  ///
  /// This does not include coordinates or attributes, but only all named
  /// entities (which can consist of various combinations of values, variances,
  /// and sparse coordinates).
  index size() const noexcept { return scipp::size(m_data); }
  /// Return true if there are 0 data items in the dataset.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  void clear();

  CoordsConstProxy coords() const noexcept;
  CoordsProxy coords() noexcept;

  LabelsConstProxy labels() const noexcept;
  LabelsProxy labels() noexcept;

  AttrsConstProxy attrs() const noexcept;
  AttrsProxy attrs() noexcept;

  bool contains(const std::string &name) const noexcept;

  void erase(const std::string_view name);

  auto find() const && = delete;
  auto find() && = delete;
  auto find(const std::string &name) & noexcept {
    return boost::make_transform_iterator(m_data.find(name),
                                          detail::make_item{this});
  }
  auto find(const std::string &name) const &noexcept {
    return boost::make_transform_iterator(m_data.find(name),
                                          detail::make_item{this});
  }

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
  void setAttr(const std::string &attrName, Variable attr);
  void setData(const std::string &name, Variable data);
  void setData(const std::string &name, const DataConstProxy &data);
  void setSparseCoord(const std::string &name, Variable coord);
  void setSparseLabels(const std::string &name, const std::string &labelName,
                       Variable labels);

  void setCoord(const Dim dim, const VariableConstProxy &coord) {
    setCoord(dim, Variable(coord));
  }
  void setLabels(const std::string &labelName,
                 const VariableConstProxy &labels) {
    setLabels(labelName, Variable(labels));
  }
  void setAttr(const std::string &attrName, const VariableConstProxy &attr) {
    setAttr(attrName, Variable(attr));
  }
  void setData(const std::string &name, const VariableConstProxy &data) {
    setData(name, Variable(data));
  }
  void setSparseCoord(const std::string &name,
                      const VariableConstProxy &coord) {
    setSparseCoord(name, Variable(coord));
  }
  void setSparseLabels(const std::string &name, const std::string &labelName,
                       const VariableConstProxy &labels) {
    setSparseLabels(name, labelName, Variable(labels));
  }

  void eraseCoord(const Dim dim);
  void eraseLabels(const std::string &labelName);
  void eraseAttr(const std::string &attrName);
  void eraseSparseCoord(const std::string &name);
  void eraseSparseLabels(const std::string &name, const std::string &labelName);

  DatasetConstProxy slice(const Slice slice1) const &;
  DatasetConstProxy slice(const Slice slice1, const Slice slice2) const &;
  DatasetConstProxy slice(const Slice slice1, const Slice slice2,
                          const Slice slice3) const &;
  DatasetProxy slice(const Slice slice1) &;
  DatasetProxy slice(const Slice slice1, const Slice slice2) &;
  DatasetProxy slice(const Slice slice1, const Slice slice2,
                     const Slice slice3) &;
  Dataset slice(const Slice slice1) const &&;
  Dataset slice(const Slice slice1, const Slice slice2) const &&;
  Dataset slice(const Slice slice1, const Slice slice2,
                const Slice slice3) const &&;

  void rename(const Dim from, const Dim to);

  bool operator==(const Dataset &other) const;
  bool operator==(const DatasetConstProxy &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const DatasetConstProxy &other) const;

  Dataset &operator+=(const DataConstProxy &other);
  Dataset &operator-=(const DataConstProxy &other);
  Dataset &operator*=(const DataConstProxy &other);
  Dataset &operator/=(const DataConstProxy &other);
  Dataset &operator+=(const VariableConstProxy &other);
  Dataset &operator-=(const VariableConstProxy &other);
  Dataset &operator*=(const VariableConstProxy &other);
  Dataset &operator/=(const VariableConstProxy &other);
  Dataset &operator+=(const DatasetConstProxy &other);
  Dataset &operator-=(const DatasetConstProxy &other);
  Dataset &operator*=(const DatasetConstProxy &other);
  Dataset &operator/=(const DatasetConstProxy &other);
  Dataset &operator+=(const Dataset &other);
  Dataset &operator-=(const Dataset &other);
  Dataset &operator*=(const Dataset &other);
  Dataset &operator/=(const Dataset &other);
  std::unordered_map<Dim, scipp::index> dimensions() const;

private:
  friend class DatasetConstProxy;
  friend class DatasetProxy;
  friend class DataConstProxy;
  friend class DataProxy;

  void setExtent(const Dim dim, const scipp::index extent, const bool isCoord);
  void setDims(const Dimensions &dims, const Dim coordDim = Dim::Invalid);
  void rebuildDims();

  std::unordered_map<Dim, scipp::index> m_dims;
  std::unordered_map<Dim, Variable> m_coords;
  std::unordered_map<std::string, Variable> m_labels;
  std::unordered_map<std::string, Variable> m_attrs;
  detail::dataset_item_map m_data;
};

/// Common functionality for other const-proxy classes.
template <class Id, class Key> class ConstProxy {
private:
  struct make_item {
    const ConstProxy *proxy;
    template <class T> auto operator()(const T &item) const {
      return std::pair<Key, VariableConstProxy>(
          item.first, detail::makeSlice(*item.second.first, proxy->slices()));
    }
  };

public:
  using key_type = Key;
  using mapped_type = Variable;

  ConstProxy(
      std::unordered_map<Key, std::pair<const Variable *, Variable *>> &&items,
      const std::vector<std::pair<Slice, scipp::index>> &slices = {})
      : m_items(std::move(items)), m_slices(slices) {
    // TODO This is very similar to the code in makeProxyItems(), provided that
    // we can give a good definion of the `dims` argument (roughly the space
    // spanned by all coords, excluding the dimensions that are sliced away).
    // Remove any items for a non-range sliced dimension. Identified via the
    // item in case of coords, or via the inner dimension in case of labels and
    // attributes.
    for (const auto &s : m_slices) {
      const auto slice = s.first;
      if (!slice.isRange()) { // The slice represents a point not a range.
                              // Dimension removed.
        for (auto it = m_items.begin(); it != m_items.end();) {
          auto erase = [slice](const auto it2) {
            if constexpr (std::is_same_v<Key, Dim>)
              return (it2->first == slice.dim());
            else
              return !it2->second.first->dims().empty() &&
                     (it2->second.first->dims().inner() == slice.dim());
          };
          if (erase(it))
            it = m_items.erase(it);
          else
            ++it;
        }
      }
    }
  }

  /// Return the number of coordinates in the proxy.
  index size() const noexcept { return scipp::size(m_items); }
  /// Return true if there are 0 coordinates in the proxy.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  /// Returns whether a given key is present in the proxy.
  bool contains(const Key &k) const {
    return m_items.find(k) != m_items.cend();
  }

  /// Return a const proxy to the coordinate for given dimension.
  VariableConstProxy operator[](const Key key) const {
    return detail::makeSlice(*m_items.at(key).first, m_slices);
  }

  auto find(const Key k) const && = delete;
  auto find(const Key k) const &noexcept {
    return boost::make_transform_iterator(m_items.find(k), make_item{this});
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

  ConstProxy slice(const Slice slice1) const {
    auto slices = m_slices;
    if constexpr (std::is_same_v<Key, Dim>) {
      const auto &coord = *m_items.at(slice1.dim()).first;
      slices.emplace_back(slice1, coord.dims()[slice1.dim()]);
    } else {
      throw std::runtime_error("TODO");
    }
    auto items = m_items;
    return ConstProxy(std::move(items), slices);
  }

  ConstProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }
  ConstProxy slice(const Slice slice1, const Slice slice2,
                   const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  bool operator==(const ConstProxy &other) const {
    if (size() != other.size())
      return false;
    for (const auto & [ name, data ] : *this) {
      try {
        if (data != other[name])
          return false;
      } catch (std::out_of_range &) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const ConstProxy &other) const { return !operator==(other); }

  const auto &items() const noexcept { return m_items; }
  const auto &slices() const noexcept { return m_slices; }

protected:
  std::unordered_map<Key, std::pair<const Variable *, Variable *>> m_items;
  std::vector<std::pair<Slice, scipp::index>> m_slices;
};

/// Common functionality for other proxy classes.
template <class Base> class MutableProxy : public Base {
private:
  struct make_item {
    const MutableProxy<Base> *proxy;
    template <class T> auto operator()(const T &item) const {
      return std::pair<typename Base::key_type, VariableProxy>(
          item.first, detail::makeSlice(*item.second.second, proxy->slices()));
    }
  };

  MutableProxy(Dataset *parent, const std::string *name, Base &&base)
      : Base(std::move(base)), m_parent(parent), m_name(name) {}

  Dataset *m_parent;
  const std::string *m_name;

public:
  MutableProxy(
      Dataset *parent, const std::string *name,
      std::unordered_map<typename Base::key_type,
                         std::pair<const Variable *, Variable *>> &&items,
      const std::vector<std::pair<Slice, scipp::index>> &slices = {})
      : Base(std::move(items), slices), m_parent(parent), m_name(name) {}

  /// Return a proxy to the coordinate for given dimension.
  VariableProxy operator[](const typename Base::key_type key) const {
    return detail::makeSlice(*Base::items().at(key).second, Base::slices());
  }

  template <class T> auto find(const T k) const && = delete;
  template <class T> auto find(const T k) const &noexcept {
    return boost::make_transform_iterator(Base::items().find(k),
                                          make_item{this});
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

  MutableProxy slice(const Slice slice1) const {
    // parent = nullptr since adding coords via slice is not supported.
    return MutableProxy(nullptr, m_name, Base::slice(slice1));
  }

  MutableProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  MutableProxy slice(const Slice slice1, const Slice slice2,
                     const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  void set(const typename Base::key_type key, const VariableConstProxy &var) {
    if (!m_parent || !Base::m_slices.empty())
      throw std::runtime_error(
          "Cannot add coord/labels/attr field to a slice.");
    if (var.dims().sparse()) {
      if (!m_name)
        throw std::runtime_error("Sparse coord/labels/attr must be added to "
                                 "coords of dataset items, not coords of "
                                 "dataset.");
      if constexpr (std::is_same_v<Base, CoordsConstProxy>)
        m_parent->setSparseCoord(*m_name, var);
      if constexpr (std::is_same_v<Base, LabelsConstProxy>)
        m_parent->setSparseLabels(*m_name, key, var);
      if constexpr (std::is_same_v<Base, AttrsConstProxy>)
        throw std::runtime_error("Attributes cannot be sparse.");
    } else {
      // TODO Would like to add coords for DataArray, as a temporary hack we
      // allow adding dense coords of the parent size is 1.
      if (m_name && m_parent->size() != 1)
        throw std::runtime_error(
            "Dense coord/labels/attr must be added to "
            "coords of dataset, not coords of dataset items.");
      if constexpr (std::is_same_v<Base, CoordsConstProxy>)
        m_parent->setCoord(key, var);
      if constexpr (std::is_same_v<Base, LabelsConstProxy>)
        m_parent->setLabels(key, var);
      if constexpr (std::is_same_v<Base, AttrsConstProxy>)
        m_parent->setAttr(key, var);
    }
    // TODO rebuild *this?!
  }

  void erase(const typename Base::key_type key) {
    if (!m_parent || !Base::m_slices.empty())
      throw std::runtime_error(
          "Cannot remove coord/labels/attr field from a slice.");

    bool sparse = m_name; // Does proxy points on sparse data or not
    if (sparse)
      sparse &= (*m_parent)[*m_name].dims().sparse();

    if (!sparse) {
      if constexpr (std::is_same_v<Base, CoordsConstProxy>)
        m_parent->eraseCoord(key);
      if constexpr (std::is_same_v<Base, LabelsConstProxy>)
        m_parent->eraseLabels(key);
      if constexpr (std::is_same_v<Base, AttrsConstProxy>)
        m_parent->eraseAttr(key);
    } else {
      if constexpr (std::is_same_v<Base, CoordsConstProxy>) {
        if (Base::m_items.count(key) == 0) {
          std::string suffix =
              Base::m_items.empty()
                  ? "no sparse coordinate defined "
                  : " sparse coordinate is defined for dim " +
                        to_string(Base::m_items.begin()->first);
          throw except::SparseDataError("No coordinate with dim " +
                                        to_string(key) + " found," + suffix);
        }
        m_parent->eraseSparseCoord(*m_name);
      }
      if constexpr (std::is_same_v<Base, LabelsConstProxy>)
        m_parent->eraseSparseLabels(*m_name, key);
      if constexpr (std::is_same_v<Base, AttrsConstProxy>)
        throw std::runtime_error("Attributes cannot be sparse.");
    }
  }
};

template <class T1, class T2> auto union_(const T1 &a, const T2 &b) {
  std::map<typename T1::key_type, typename T1::mapped_type> out;

  for (const auto & [ key, item ] : a)
    out.emplace(key, item);
  for (const auto & [ key, item ] : b) {
    if (const auto it = a.find(key); it != a.end())
      expect::equals(item, it->second);
    else
      out.emplace(key, item);
  }
  return out;
}

/// Const proxy for Dataset, implementing slicing and item selection.
class SCIPP_CORE_EXPORT DatasetConstProxy {
  explicit DatasetConstProxy() : m_dataset(nullptr) {}

public:
  using key_type = std::string;
  using mapped_type = DataArray;

  DatasetConstProxy(const Dataset &dataset) : m_dataset(&dataset) {
    for (const auto &item : dataset.m_data)
      m_indices.emplace_back(item.first);
  }

  static DatasetConstProxy makeProxyWithEmptyIndexes(const Dataset &dataset) {
    auto res = DatasetConstProxy();
    res.m_dataset = &dataset;
    return res;
  }

  index size() const noexcept { return m_indices.size(); }
  [[nodiscard]] bool empty() const noexcept { return m_indices.empty(); }

  CoordsConstProxy coords() const noexcept;
  LabelsConstProxy labels() const noexcept;
  AttrsConstProxy attrs() const noexcept;

  bool contains(const std::string &name) const noexcept;

  DataConstProxy operator[](const std::string &name) const;

  auto find(const std::string &name) const && = delete;
  auto find(const std::string &name) const &noexcept {
    return boost::make_transform_iterator(
        std::find(std::begin(m_indices), std::end(m_indices), name),
        detail::make_item{this});
  }

  auto begin() const && = delete;
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_indices.begin(),
                                          detail::make_item{this});
  }
  auto end() const && = delete;
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_indices.end(),
                                          detail::make_item{this});
  }

  /// Return a slice of the dataset proxy.
  ///
  /// The returned proxy will not contain references to data items that do not
  /// depend on the sliced dimension.
  DatasetConstProxy slice(const Slice slice1) const {
    const auto currentDims = dimensions();
    expect::validSlice(currentDims, slice1);
    DatasetConstProxy sliced(*this);
    auto &indices = sliced.m_indices;
    sliced.m_indices.erase(
        std::remove_if(indices.begin(), indices.end(),
                       [&slice1, this](const auto &index) {
                         return !(*this)[index].dims().contains(slice1.dim());
                       }),
        indices.end());
    // The dimension extent is either given by the coordinate, or by data, which
    // can be 1 shorter in case of a bin-edge coordinate.
    scipp::index extent = currentDims.at(slice1.dim());
    for (const auto item : *this)
      if (item.second.dims().contains(slice1.dim()) &&
          item.second.dims()[slice1.dim()] == extent - 1) {
        --extent;
        break;
      }
    sliced.m_slices.emplace_back(slice1, extent);
    return sliced;
  }

  DatasetConstProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  DatasetConstProxy slice(const Slice slice1, const Slice slice2,
                          const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  const auto &slices() const noexcept { return m_slices; }
  const auto &dataset() const noexcept { return *m_dataset; }

  bool operator==(const Dataset &other) const;
  bool operator==(const DatasetConstProxy &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const DatasetConstProxy &other) const;
  std::unordered_map<Dim, scipp::index> dimensions() const;

private:
  const Dataset *m_dataset;

protected:
  void expectValidKey(const std::string &name) const;
  std::vector<std::string> m_indices;
  std::vector<std::pair<Slice, scipp::index>> m_slices;
};

/// Proxy for Dataset, implementing slicing and item selection.
class SCIPP_CORE_EXPORT DatasetProxy : public DatasetConstProxy {
private:
  DatasetProxy(DatasetConstProxy &&base, Dataset *dataset)
      : DatasetConstProxy(std::move(base)), m_mutableDataset(dataset) {}

public:
  DatasetProxy(Dataset &dataset)
      : DatasetConstProxy(dataset), m_mutableDataset(&dataset) {}

  CoordsProxy coords() const noexcept;
  LabelsProxy labels() const noexcept;
  AttrsProxy attrs() const noexcept;

  DataProxy operator[](const std::string &name) const;

  auto find(const std::string &name) const && = delete;
  auto find(const std::string &name) const &noexcept {
    return boost::make_transform_iterator(
        std::find(std::begin(m_indices), std::end(m_indices), name),
        detail::make_item{this});
  }

  auto begin() const && = delete;
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_indices.begin(),
                                          detail::make_item{this});
  }
  auto end() const && = delete;
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_indices.end(),
                                          detail::make_item{this});
  }

  DatasetProxy slice(const Slice slice1) const {
    return {DatasetConstProxy::slice(slice1), m_mutableDataset};
  }

  DatasetProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  DatasetProxy slice(const Slice slice1, const Slice slice2,
                     const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  DatasetProxy operator+=(const DataConstProxy &other) const;
  DatasetProxy operator-=(const DataConstProxy &other) const;
  DatasetProxy operator*=(const DataConstProxy &other) const;
  DatasetProxy operator/=(const DataConstProxy &other) const;
  DatasetProxy operator+=(const VariableConstProxy &other) const;
  DatasetProxy operator-=(const VariableConstProxy &other) const;
  DatasetProxy operator*=(const VariableConstProxy &other) const;
  DatasetProxy operator/=(const VariableConstProxy &other) const;
  DatasetProxy operator+=(const DatasetConstProxy &other) const;
  DatasetProxy operator-=(const DatasetConstProxy &other) const;
  DatasetProxy operator*=(const DatasetConstProxy &other) const;
  DatasetProxy operator/=(const DatasetConstProxy &other) const;
  DatasetProxy operator+=(const Dataset &other) const;
  DatasetProxy operator-=(const Dataset &other) const;
  DatasetProxy operator*=(const Dataset &other) const;
  DatasetProxy operator/=(const Dataset &other) const;

  DatasetProxy assign(const DatasetConstProxy &other) const;

private:
  Dataset *m_mutableDataset;
};

SCIPP_CORE_EXPORT DataArray copy(const DataConstProxy &array);
SCIPP_CORE_EXPORT Dataset copy(const DatasetConstProxy &dataset);

/// Data array, a variable with coordinates, labels, and attributes.
class SCIPP_CORE_EXPORT DataArray {
public:
  DataArray() = default;
  explicit DataArray(const DataConstProxy &proxy);
  template <class CoordMap = std::map<Dim, Variable>,
            class LabelsMap = std::map<std::string, Variable>,
            class AttrMap = std::map<std::string, Variable>>
  DataArray(std::optional<Variable> data, CoordMap coords = {},
            LabelsMap labels = {}, AttrMap attrs = {},
            const std::string &name = "") {
    if (data)
      m_holder.setData(name, std::move(*data));
    for (auto && [ dim, c ] : coords)
      if (c.dims().sparse())
        m_holder.setSparseCoord(name, std::move(c));
      else
        m_holder.setCoord(dim, std::move(c));
    for (auto && [ label_name, l ] : labels)
      if (l.dims().sparse())
        m_holder.setSparseLabels(name, std::string(label_name), std::move(l));
      else
        m_holder.setLabels(std::string(label_name), std::move(l));
    for (auto && [ attr_name, a ] : attrs)
      m_holder.setAttr(std::string(attr_name), std::move(a));
    if (m_holder.size() != 1)
      throw std::runtime_error(
          "DataArray must have either data or a sparse coordinate.");
  }

  explicit operator bool() const noexcept { return !m_holder.empty(); }
  operator DataConstProxy() const;
  operator DataProxy();

  const std::string &name() const { return m_holder.begin()->first; }

  CoordsConstProxy coords() const { return get().coords(); }
  CoordsProxy coords() { return get().coords(); }

  LabelsConstProxy labels() const { return get().labels(); }
  LabelsProxy labels() { return get().labels(); }

  AttrsConstProxy attrs() const { return get().attrs(); }
  AttrsProxy attrs() { return get().attrs(); }

  Dimensions dims() const { return get().dims(); }
  DType dtype() const { return get().dtype(); }
  units::Unit unit() const { return get().unit(); }

  void setUnit(const units::Unit unit) { get().setUnit(unit); }

  template <class T> void setVariances(Vector<T> &&v) {
    get().setVariances(std::move(v));
  }

  /// Return true if the data array contains data values.
  bool hasData() const { return get().hasData(); }
  /// Return true if the data array contains data variances.
  bool hasVariances() const { return get().hasVariances(); }

  /// Return untyped const proxy for data (values and optional variances).
  VariableConstProxy data() const { return get().data(); }
  /// Return untyped proxy for data (values and optional variances).
  VariableProxy data() { return get().data(); }

  /// Return typed const proxy for data values.
  template <class T> auto values() const { return get().values<T>(); }
  /// Return typed proxy for data values.
  template <class T> auto values() { return get().values<T>(); }

  /// Return typed const proxy for data variances.
  template <class T> auto variances() const { return get().variances<T>(); }
  /// Return typed proxy for data variances.
  template <class T> auto variances() { return get().variances<T>(); }

  DataArray &operator+=(const DataConstProxy &other);
  DataArray &operator-=(const DataConstProxy &other);
  DataArray &operator*=(const DataConstProxy &other);
  DataArray &operator/=(const DataConstProxy &other);
  DataArray &operator+=(const VariableConstProxy &other);
  DataArray &operator-=(const VariableConstProxy &other);
  DataArray &operator*=(const VariableConstProxy &other);
  DataArray &operator/=(const VariableConstProxy &other);

  // TODO need to define some details regarding handling of dense coords in case
  // the array is sparse, not exposing this to Python for now.
  void setCoord(const Dim dim, Variable coord) {
    m_holder.setCoord(dim, std::move(coord));
  }
  void setCoord(const Dim dim, const VariableConstProxy &coord) {
    setCoord(dim, Variable(coord));
  }

  DataConstProxy slice(const Slice slice1) const & {
    return get().slice(slice1);
  }
  DataConstProxy slice(const Slice slice1, const Slice slice2) const & {
    return get().slice(slice1, slice2);
  }
  DataConstProxy slice(const Slice slice1, const Slice slice2,
                       const Slice slice3) const & {
    return get().slice(slice1, slice2, slice3);
  }
  DataProxy slice(const Slice slice1) & { return get().slice(slice1); }
  DataProxy slice(const Slice slice1, const Slice slice2) & {
    return get().slice(slice1, slice2);
  }
  DataProxy slice(const Slice slice1, const Slice slice2,
                  const Slice slice3) & {
    return get().slice(slice1, slice2, slice3);
  }
  DataArray slice(const Slice slice1) const && {
    return copy(get().slice(slice1));
  }
  DataArray slice(const Slice slice1, const Slice slice2) const && {
    return copy(get().slice(slice1, slice2));
  }
  DataArray slice(const Slice slice1, const Slice slice2,
                  const Slice slice3) const && {
    return copy(get().slice(slice1, slice2, slice3));
  }

private:
  DataConstProxy get() const;
  DataProxy get();

  Dataset m_holder;
};

SCIPP_CORE_EXPORT DataArray operator+(const DataConstProxy &a,
                                      const DataConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator-(const DataConstProxy &a,
                                      const DataConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator*(const DataConstProxy &a,
                                      const DataConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator/(const DataConstProxy &a,
                                      const DataConstProxy &b);

SCIPP_CORE_EXPORT DataArray operator+(const DataConstProxy &a,
                                      const VariableConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator-(const DataConstProxy &a,
                                      const VariableConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator*(const DataConstProxy &a,
                                      const VariableConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator/(const DataConstProxy &a,
                                      const VariableConstProxy &b);

SCIPP_CORE_EXPORT DataArray operator+(const VariableConstProxy &a,
                                      const DataConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator-(const VariableConstProxy &a,
                                      const DataConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator*(const VariableConstProxy &a,
                                      const DataConstProxy &b);
SCIPP_CORE_EXPORT DataArray operator/(const VariableConstProxy &a,
                                      const DataConstProxy &b);

SCIPP_CORE_EXPORT Dataset operator+(const Dataset &lhs, const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const Dataset &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const Dataset &lhs,
                                    const DataConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DatasetConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DatasetConstProxy &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DatasetConstProxy &lhs,
                                    const DataConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DataConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DataConstProxy &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const Dataset &lhs,
                                    const VariableConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const VariableConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DatasetProxy &lhs,
                                    const VariableConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const VariableConstProxy &lhs,
                                    const DatasetProxy &rhs);

SCIPP_CORE_EXPORT Dataset operator-(const Dataset &lhs, const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const Dataset &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const Dataset &lhs,
                                    const DataConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DatasetConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DatasetConstProxy &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DatasetConstProxy &lhs,
                                    const DataConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DataConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DataConstProxy &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const Dataset &lhs,
                                    const VariableConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const VariableConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DatasetProxy &lhs,
                                    const VariableConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const VariableConstProxy &lhs,
                                    const DatasetProxy &rhs);

SCIPP_CORE_EXPORT Dataset operator*(const Dataset &lhs, const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const Dataset &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const Dataset &lhs,
                                    const DataConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DatasetConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DatasetConstProxy &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DatasetConstProxy &lhs,
                                    const DataConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DataConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DataConstProxy &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const Dataset &lhs,
                                    const VariableConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const VariableConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DatasetProxy &lhs,
                                    const VariableConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const VariableConstProxy &lhs,
                                    const DatasetProxy &rhs);

SCIPP_CORE_EXPORT Dataset operator/(const Dataset &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const Dataset &lhs, const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const Dataset &lhs,
                                    const DataConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DatasetConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DatasetConstProxy &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DatasetConstProxy &lhs,
                                    const DataConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DataConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DataConstProxy &lhs,
                                    const DatasetConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const Dataset &lhs,
                                    const VariableConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const VariableConstProxy &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DatasetProxy &lhs,
                                    const VariableConstProxy &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const VariableConstProxy &lhs,
                                    const DatasetProxy &rhs);

SCIPP_CORE_EXPORT DataArray histogram(const DataConstProxy &sparse,
                                      const Variable &binEdges);
SCIPP_CORE_EXPORT DataArray histogram(const DataConstProxy &sparse,
                                      const VariableConstProxy &binEdges);
SCIPP_CORE_EXPORT Dataset histogram(const Dataset &dataset,
                                    const VariableConstProxy &bins);
SCIPP_CORE_EXPORT Dataset histogram(const Dataset &dataset,
                                    const Variable &bins);
SCIPP_CORE_EXPORT Dataset histogram(const Dataset &dataset, const Dim &dim);

SCIPP_CORE_EXPORT Dataset merge(const DatasetConstProxy &a,
                                const DatasetConstProxy &b);

SCIPP_CORE_EXPORT DataArray sum(const DataConstProxy &a, const Dim dim);
SCIPP_CORE_EXPORT Dataset sum(const DatasetConstProxy &d, const Dim dim);

SCIPP_CORE_EXPORT DataArray mean(const DataConstProxy &a, const Dim dim);
SCIPP_CORE_EXPORT Dataset mean(const DatasetConstProxy &d, const Dim dim);

SCIPP_CORE_EXPORT DataArray concatenate(const DataConstProxy &a,
                                        const DataConstProxy &b, const Dim dim);
SCIPP_CORE_EXPORT Dataset concatenate(const DatasetConstProxy &a,
                                      const DatasetConstProxy &b,
                                      const Dim dim);

SCIPP_CORE_EXPORT DataArray rebin(const DataConstProxy &a, const Dim dim,
                                  const VariableConstProxy &coord);
SCIPP_CORE_EXPORT Dataset rebin(const DatasetConstProxy &d, const Dim dim,
                                const VariableConstProxy &coord);

SCIPP_CORE_EXPORT VariableConstProxy same(const VariableConstProxy &a,
                                          const VariableConstProxy &b);

} // namespace scipp::core

#endif // SCIPP_DATASET_H
