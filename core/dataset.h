// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DATASET_NEXT_H
#define DATASET_NEXT_H

#include <functional>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>

#include <boost/iterator/transform_iterator.hpp>

#include "dimension.h"
#include "except.h"
#include "variable.h"

namespace scipp::core {

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
using LabelsConstProxy = ConstProxy<ProxyId::Labels, std::string_view>;
/// Proxy for accessing labels of Dataset and DataProxy.
using LabelsProxy = MutableProxy<LabelsConstProxy>;
/// Proxy for accessing attributes of const Dataset and DataConstProxy.
using AttrsConstProxy = ConstProxy<ProxyId::Attrs, std::string_view>;
/// Proxy for accessing attributes of Dataset and DataProxy.
using AttrsProxy = MutableProxy<AttrsConstProxy>;

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

template <class Var>
auto makeSlice(Var &var,
               const std::vector<std::pair<Slice, scipp::index>> &slices) {
  std::conditional_t<std::is_const_v<Var>, ConstVariableSlice, VariableSlice>
      slice(var);
  for (const auto[params, extent] : slices) {
    const auto[dim, begin, end] = params;
    if (slice.dimensions().contains(dim))
      slice = slice(dim, begin, end + slice.dimensions()[dim] - extent);
  }
  return slice;
}
} // namespace detail

/// Const proxy for a data item and related coordinates of Dataset.
class DataConstProxy {
public:
  DataConstProxy(const Dataset &dataset, const detail::DatasetData &data,
                 const std::vector<std::pair<Slice, scipp::index>> &slices = {})
      : m_dataset(&dataset), m_data(&data), m_slices(slices) {}

  bool isSparse() const noexcept;
  Dim sparseDim() const noexcept;
  Dimensions dims() const noexcept;
  units::Unit unit() const;

  CoordsConstProxy coords() const noexcept;
  LabelsConstProxy labels() const noexcept;
  AttrsConstProxy attrs() const noexcept;

  /// Return true if the proxy contains data values.
  bool hasValues() const noexcept { return m_data->values.has_value(); }
  /// Return true if the proxy contains data variances.
  bool hasVariances() const noexcept { return m_data->variances.has_value(); }

  /// Return untyped const proxy for data values.
  ConstVariableSlice values() const {
    return detail::makeSlice(*m_data->values, slices());
  }
  /// Return typed const proxy for data values.
  template <class T> auto values() const { return values().template span<T>(); }

  /// Return untyped const proxy for data variances.
  ConstVariableSlice variances() const {
    return detail::makeSlice(*m_data->variances, slices());
  }
  /// Return typed const proxy for data variances.
  template <class T> auto variances() const {
    return variances().template span<T>();
  }

  DataConstProxy slice(const Slice slice1) const {
    expect::validSlice(dims(), slice1);
    auto tmp(m_slices);
    tmp.emplace_back(slice1, dims()[slice1.dim]);
    return {*m_dataset, *m_data, std::move(tmp)};
  }

  DataConstProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  DataConstProxy slice(const Slice slice1, const Slice slice2,
                       const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  bool operator==(const DataConstProxy &other) const;
  bool operator!=(const DataConstProxy &other) const {
    return !operator==(other);
  }

  const std::vector<std::pair<Slice, scipp::index>> &slices() const noexcept {
    return m_slices;
  }

private:
  friend class DatasetConstProxy;
  friend class DatasetProxy;

  const Dataset *m_dataset;
  const detail::DatasetData *m_data;
  std::vector<std::pair<Slice, scipp::index>> m_slices;
};

/// Proxy for a data item and related coordinates of Dataset.
class DataProxy : public DataConstProxy {
public:
  DataProxy(Dataset &dataset, detail::DatasetData &data,
            const std::vector<std::pair<Slice, scipp::index>> &slices = {})
      : DataConstProxy(dataset, data, slices), m_mutableDataset(&dataset),
        m_mutableData(&data) {}

  CoordsProxy coords() const noexcept;
  LabelsProxy labels() const noexcept;
  AttrsProxy attrs() const noexcept;

  /// Return untyped proxy for data values.
  VariableSlice values() const {
    return detail::makeSlice(*m_mutableData->values, slices());
  }
  /// Return typed proxy for data values.
  template <class T> auto values() const { return values().template span<T>(); }

  /// Return untyped proxy for data variances.
  VariableSlice variances() const {
    return detail::makeSlice(*m_mutableData->variances, slices());
  }
  /// Return typed proxy for data variances.
  template <class T> auto variances() const {
    return variances().template span<T>();
  }

  DataProxy slice(const Slice slice1) const {
    expect::validSlice(dims(), slice1);
    auto tmp(slices());
    tmp.emplace_back(slice1, dims()[slice1.dim]);
    return {*m_mutableDataset, *m_mutableData, std::move(tmp)};
  }

  DataProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  DataProxy slice(const Slice slice1, const Slice slice2,
                  const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
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
    if constexpr (std::is_same_v<std::remove_const_t<D>, Dataset>)
      return {item.first, P(*dataset, item.second)};
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

  AttrsConstProxy attrs() const noexcept;
  AttrsProxy attrs() noexcept;

  DataConstProxy operator[](const std::string_view name) const;
  DataProxy operator[](const std::string_view name);

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
  void setValues(const std::string &name, Variable values);
  void setVariances(const std::string &name, Variable variances);
  void setSparseCoord(const std::string &name, Variable coord);
  void setSparseLabels(const std::string &name, const std::string &labelName,
                       Variable labels);

  DatasetConstProxy slice(const Slice slice1) const;
  DatasetConstProxy slice(const Slice slice1, const Slice slice2) const;
  DatasetConstProxy slice(const Slice slice1, const Slice slice2,
                          const Slice slice3) const;
  DatasetProxy slice(const Slice slice1);
  DatasetProxy slice(const Slice slice1, const Slice slice2);
  DatasetProxy slice(const Slice slice1, const Slice slice2,
                     const Slice slice3);

  bool operator==(const Dataset &other) const;
  bool operator==(const DatasetConstProxy &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const DatasetConstProxy &other) const;

private:
  friend class DatasetConstProxy;
  friend class DatasetProxy;
  friend class DataConstProxy;
  friend class DataProxy;

  void setExtent(const Dim dim, const scipp::index extent, const bool isCoord);
  void setDims(const Dimensions &dims, const Dim coordDim = Dim::Invalid);

  std::map<Dim, scipp::index> m_dims;
  std::map<Dim, Variable> m_coords;
  std::map<std::string, Variable> m_labels;
  std::map<std::string, Variable> m_attrs;
  std::map<std::string, detail::DatasetData, std::less<>> m_data;
};

/// Common functionality for other const-proxy classes.
template <class Id, class Key> class ConstProxy {
private:
  struct make_item {
    const ConstProxy *proxy;
    auto operator()(const auto &item) const {
      return std::pair<Key, ConstVariableSlice>(
          item.first, detail::makeSlice(*item.second.first, proxy->slices()));
    }
  };

public:
  using key_type = Key;

  ConstProxy(std::map<Key, std::pair<const Variable *, Variable *>> &&items,
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
      if (slice.end == -1) {
        for (auto it = m_items.begin(); it != m_items.end();) {
          auto erase = [slice](const auto it) {
            if constexpr (std::is_same_v<Key, Dim>)
              return (it->first == slice.dim);
            else
              return !it->second.first->dims().empty() &&
                     (it->second.first->dims().inner() == slice.dim);
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

  /// Return a const proxy to the coordinate for given dimension.
  ConstVariableSlice operator[](const Key key) const {
    return detail::makeSlice(*m_items.at(key).first, m_slices);
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
    const auto &coord = *m_items.at(slice1.dim).first;
    auto slices = m_slices;
    slices.emplace_back(slice1, coord.dimensions()[slice1.dim]);
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
  std::map<Key, std::pair<const Variable *, Variable *>> m_items;
  std::vector<std::pair<Slice, scipp::index>> m_slices;
};

/// Common functionality for other proxy classes.
template <class Base> class MutableProxy : public Base {
private:
  struct make_item {
    const MutableProxy<Base> *proxy;
    auto operator()(const auto &item) const {
      return std::pair<typename Base::key_type, VariableSlice>(
          item.first, detail::makeSlice(*item.second.second, proxy->slices()));
    }
  };

  explicit MutableProxy(Base &&base) : Base(std::move(base)) {}

public:
  using Base::Base;

  /// Return a proxy to the coordinate for given dimension.
  VariableSlice operator[](const typename Base::key_type key) const {
    return detail::makeSlice(*Base::items().at(key).second, Base::slices());
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
    return MutableProxy(Base::slice(slice1));
  }

  MutableProxy slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  MutableProxy slice(const Slice slice1, const Slice slice2,
                     const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }
};

/// Const proxy for Dataset, implementing slicing and item selection.
class DatasetConstProxy {
public:
  explicit DatasetConstProxy(const Dataset &dataset) : m_dataset(&dataset) {
    for (const auto &item : dataset.m_data)
      m_indices.emplace_back(item.first);
  }

  index size() const noexcept { return m_indices.size(); }
  [[nodiscard]] bool empty() const noexcept { return m_indices.empty(); }

  CoordsConstProxy coords() const noexcept;
  LabelsConstProxy labels() const noexcept;
  AttrsConstProxy attrs() const noexcept;
  DataConstProxy operator[](const std::string_view name) const;

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
    DatasetConstProxy sliced(*this);
    auto &indices = sliced.m_indices;
    sliced.m_indices.erase(
        std::remove_if(indices.begin(), indices.end(),
                       [&slice1, this](const auto &index) {
                         return !(*this)[index].dims().contains(slice1.dim);
                       }),
        indices.end());
    // The dimension extent is either given by the coordinate, or by data, which
    // can be 1 shorter in case of a bin-edge coordinate.
    scipp::index extent = coords()[slice1.dim].dims()[slice1.dim];
    for (const auto item : *this)
      if (item.second.dims().contains(slice1.dim) &&
          item.second.dims()[slice1.dim] == extent - 1) {
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

  bool operator==(const Dataset &other) const;
  bool operator==(const DatasetConstProxy &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const DatasetConstProxy &other) const;

private:
  const Dataset *m_dataset;

protected:
  void expectValidKey(const std::string_view name) const;
  std::vector<std::string> m_indices;
  std::vector<std::pair<Slice, scipp::index>> m_slices;
};

/// Proxy for Dataset, implementing slicing and item selection.
class DatasetProxy : public DatasetConstProxy {
private:
  DatasetProxy(DatasetConstProxy &&base, Dataset *dataset)
      : DatasetConstProxy(std::move(base)), m_mutableDataset(dataset) {}

public:
  explicit DatasetProxy(Dataset &dataset)
      : DatasetConstProxy(dataset), m_mutableDataset(&dataset) {}

  CoordsProxy coords() const noexcept;
  LabelsProxy labels() const noexcept;
  AttrsProxy attrs() const noexcept;
  DataProxy operator[](const std::string_view name) const;

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

private:
  Dataset *m_mutableDataset;
};

std::ostream &operator<<(std::ostream &os, const DataConstProxy &data);
std::ostream &operator<<(std::ostream &os, const DataProxy &data);
std::ostream &operator<<(std::ostream &os, const DatasetConstProxy &dataset);
std::ostream &operator<<(std::ostream &os, const DatasetProxy &dataset);
std::ostream &operator<<(std::ostream &os, const Dataset &dataset);
std::ostream &operator<<(std::ostream &os, const ConstVariableSlice &variable);
std::ostream &operator<<(std::ostream &os, const VariableSlice &variable);
std::ostream &operator<<(std::ostream &os, const Dim dim);

} // namespace scipp::core

#endif // DATASET_NEXT_H
