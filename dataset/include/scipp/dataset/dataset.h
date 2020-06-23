// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <unordered_map>

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/common/deep_ptr.h"
#include "scipp/dataset/dataset_access.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/map_view.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {

class DataArray;
class Dataset;
class DatasetConstView;
class DatasetView;
struct UnalignedData;

namespace detail {
/// Helper for holding data items in Dataset.
struct DatasetData {
  /// Optional data values (with optional variances).
  Variable data;
  /// Unaligned data, a simple struct of aligned dimensions alongside a data
  /// array with unaligned content.
  deep_ptr<UnalignedData> unaligned;
  /// Attributes for data.
  std::unordered_map<std::string, Variable> attrs;
};

using dataset_item_map = std::unordered_map<std::string, DatasetData>;
} // namespace detail

/// Policies for attribute propagation in operations with data arrays or
/// dataset.
enum class AttrPolicy { Keep, Drop };

/// Const view for a data item and related coordinates of Dataset.
class SCIPP_DATASET_EXPORT DataArrayConstView {
public:
  using value_type = DataArray;
  DataArrayConstView() = default;
  DataArrayConstView(const Dataset &dataset,
                     const detail::dataset_item_map::value_type &data,
                     const detail::slice_list &slices = {},
                     VariableView &&view = VariableView{});

  explicit operator bool() const noexcept { return m_dataset != nullptr; }

  const std::string &name() const noexcept;

  Dimensions dims() const noexcept;
  DType dtype() const;
  units::Unit unit() const;

  CoordsConstView coords() const noexcept;
  AttrsConstView attrs() const noexcept;
  MasksConstView masks() const noexcept;

  /// Return true if the view contains data values.
  bool hasData() const noexcept { return static_cast<bool>(m_view); }
  /// Return true if the view contains data variances.
  bool hasVariances() const noexcept {
    return hasData() ? data().hasVariances() : unaligned().hasVariances();
  }

  /// Return untyped const view for data (values and optional variances).
  const VariableConstView &data() const {
    if (!hasData())
      throw except::RealignedDataError("No data in item.");
    return m_view;
  }
  /// Return typed const view for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed const view for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  DataArrayConstView unaligned() const;

  DataArrayConstView slice(const Slice s) const;

  const detail::slice_list &slices() const noexcept { return m_slices; }

  auto &underlying() const { return m_data->second; }
  Dimensions parentDims() const noexcept;

  std::vector<std::pair<Dim, Variable>> slice_bounds() const;

protected:
  // Note that m_view is a VariableView, not a VariableConstView. In case
  // *this (DataArrayConstView) is stand-alone (not part of DataArrayView),
  // m_view is actually just a VariableConstView wrapped in an (invalid)
  // VariableView. The interface guarantees that the invalid mutable view is
  // not accessible. This wrapping avoids inefficient duplication of the view in
  // the child class DataArrayView.
  VariableView m_view; // empty if the array has no (aligned) data

private:
  friend class DatasetConstView;
  friend class DatasetView;

  const Dataset *m_dataset{nullptr};
  const detail::dataset_item_map::value_type *m_data{nullptr};
  detail::slice_list m_slices;

  template <class MapView> MapView makeView() const;
};

SCIPP_DATASET_EXPORT bool operator==(const DataArrayConstView &a,
                                     const DataArrayConstView &b);
SCIPP_DATASET_EXPORT bool operator!=(const DataArrayConstView &a,
                                     const DataArrayConstView &b);

class DatasetConstView;
class DatasetView;
class Dataset;

/// View for a data item and related coordinates of Dataset.
class SCIPP_DATASET_EXPORT DataArrayView : public DataArrayConstView {
public:
  DataArrayView() = default;
  DataArrayView(Dataset &dataset, detail::dataset_item_map::value_type &data,
                const detail::slice_list &slices = {},
                VariableView &&view = VariableView{});

  CoordsView coords() const noexcept;
  MasksView masks() const noexcept;
  AttrsView attrs() const noexcept;

  void setUnit(const units::Unit unit) const;

  /// Return untyped view for data (values and optional variances).
  const VariableView &data() const {
    if (!hasData())
      throw except::RealignedDataError("No data in item.");
    return m_view;
  }
  /// Return typed view for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  DataArrayView unaligned() const;

  DataArrayView slice(const Slice s) const;

  DataArrayView assign(const DataArrayConstView &other) const;
  DataArrayView assign(const Variable &other) const;
  DataArrayView assign(const VariableConstView &other) const;

  DataArrayView operator+=(const DataArrayConstView &other) const;
  DataArrayView operator-=(const DataArrayConstView &other) const;
  DataArrayView operator*=(const DataArrayConstView &other) const;
  DataArrayView operator/=(const DataArrayConstView &other) const;
  DataArrayView operator+=(const VariableConstView &other) const;
  DataArrayView operator-=(const VariableConstView &other) const;
  DataArrayView operator*=(const VariableConstView &other) const;
  DataArrayView operator/=(const VariableConstView &other) const;

  void setData(Variable data) const;

private:
  friend class DatasetConstView;
  // For internal use in DatasetConstView.
  explicit DataArrayView(DataArrayConstView &&base)
      : DataArrayConstView(std::move(base)), m_mutableDataset{nullptr},
        m_mutableData{nullptr} {}

  Dataset *m_mutableDataset{nullptr};
  detail::dataset_item_map::value_type *m_mutableData{nullptr};

  template <class MapView> MapView makeView() const;
};

namespace detail {
template <class T> struct is_const;
template <> struct is_const<Dataset> : std::false_type {};
template <> struct is_const<const Dataset> : std::true_type {};
template <> struct is_const<const DatasetView> : std::false_type {};
template <> struct is_const<const DatasetConstView> : std::true_type {};
template <> struct is_const<DatasetView> : std::false_type {};
template <> struct is_const<DatasetConstView> : std::true_type {};

/// Helper for creating iterators of Dataset.
template <class D> struct make_item {
  D *dataset;
  using P =
      std::conditional_t<is_const<D>::value, DataArrayConstView, DataArrayView>;
  template <class T> auto operator()(T &item) const {
    if constexpr (std::is_same_v<std::remove_const_t<D>, Dataset>)
      return P(*dataset, item);
    else
      return P(dataset->dataset(), item, dataset->slices());
  }
};
template <class D> make_item(D *) -> make_item<D>;

} // namespace detail

/// Collection of data arrays.
class SCIPP_DATASET_EXPORT Dataset {
public:
  using key_type = std::string;
  using mapped_type = DataArray;
  using value_type = std::pair<const std::string &, DataArrayConstView>;
  using const_view_type = DatasetConstView;
  using view_type = DatasetView;

  Dataset() = default;
  explicit Dataset(const DatasetConstView &view);
  explicit Dataset(const DataArrayConstView &data);
  explicit Dataset(const std::map<std::string, DataArrayConstView> &data);

  template <class DataMap, class CoordMap, class MasksMap, class AttrMap>
  Dataset(DataMap data, CoordMap coords, MasksMap masks, AttrMap attrs) {
    for (auto &&[dim, coord] : coords)
      setCoord(dim, std::move(coord));
    for (auto &&[name, mask] : masks)
      setMask(std::string(name), std::move(mask));
    for (auto &&[name, attr] : attrs)
      setAttr(std::string(name), std::move(attr));
    if constexpr (std::is_same_v<std::decay_t<DataMap>, DatasetConstView>)
      for (auto &&item : data)
        setData(item.name(), item);
    else
      for (auto &&[name, item] : data)
        setData(std::string(name), std::move(item));
  }

  /// Return the number of data items in the dataset.
  ///
  /// This does not include coordinates or attributes, but only all named
  /// entities (which can consist of various combinations of values, variances,
  /// and events coordinates).
  index size() const noexcept { return scipp::size(m_data); }
  /// Return true if there are 0 data items in the dataset.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  void clear();

  CoordsConstView coords() const noexcept;
  CoordsView coords() noexcept;

  AttrsConstView attrs() const noexcept;
  AttrsView attrs() noexcept;

  MasksConstView masks() const noexcept;
  MasksView masks() noexcept;

  bool contains(const std::string &name) const noexcept;

  void erase(const std::string &name);
  [[nodiscard]] DataArray extract(const std::string &name);

  auto find() const && = delete;
  auto find() && = delete;
  auto find(const std::string &name) &noexcept {
    return boost::make_transform_iterator(m_data.find(name),
                                          detail::make_item{this});
  }
  auto find(const std::string &name) const &noexcept {
    return boost::make_transform_iterator(m_data.find(name),
                                          detail::make_item{this});
  }

  DataArrayConstView operator[](const std::string &name) const;
  DataArrayView operator[](const std::string &name);

  auto begin() const && = delete;
  auto begin() && = delete;
  /// Return const iterator to the beginning of all data items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_data.begin(),
                                          detail::make_item{this});
  }
  /// Return iterator to the beginning of all data items.
  auto begin() &noexcept {
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
  auto end() &noexcept {
    return boost::make_transform_iterator(m_data.end(),
                                          detail::make_item{this});
  }

  auto items_begin() const && = delete;
  auto items_begin() && = delete;
  auto items_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_key_value);
  }
  auto items_begin() &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_key_value);
  }
  auto items_end() const && = delete;
  auto items_end() && = delete;
  auto items_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_key_value);
  }

  auto items_end() &noexcept {
    return boost::make_transform_iterator(end(), detail::make_key_value);
  }

  auto keys_begin() const && = delete;
  auto keys_begin() && = delete;
  auto keys_begin() const &noexcept {
    return boost::make_transform_iterator(m_data.begin(), detail::make_key);
  }
  auto keys_begin() &noexcept {
    return boost::make_transform_iterator(m_data.begin(), detail::make_key);
  }
  auto keys_end() const && = delete;
  auto keys_end() && = delete;
  auto keys_end() const &noexcept {
    return boost::make_transform_iterator(m_data.end(), detail::make_key);
  }

  auto keys_end() &noexcept {
    return boost::make_transform_iterator(m_data.end(), detail::make_key);
  }

  void setCoord(const Dim dim, Variable coord);
  void setMask(const std::string &masksName, Variable masks);
  void setAttr(const std::string &attrName, Variable attr);
  void setAttr(const std::string &name, const std::string &attrName,
               Variable attr);
  void setData(const std::string &name, Variable data,
               const AttrPolicy attrPolicy = AttrPolicy::Drop);
  void setData(const std::string &name, const DataArrayConstView &data);
  void setData(const std::string &name, DataArray data);

  void setCoord(const Dim dim, const VariableConstView &coord) {
    setCoord(dim, Variable(coord));
  }
  void setMask(const std::string &masksName, const VariableConstView &mask) {
    setMask(masksName, Variable(mask));
  }
  void setAttr(const std::string &attrName, const VariableConstView &attr) {
    setAttr(attrName, Variable(attr));
  }
  void setAttr(const std::string &name, const std::string &attrName,
               const VariableConstView &attr) {
    setAttr(name, attrName, Variable(attr));
  }
  void setData(const std::string &name, const VariableConstView &data,
               const AttrPolicy attrPolicy = AttrPolicy::Drop) {
    setData(name, Variable(data), attrPolicy);
  }

  void eraseCoord(const Dim dim);
  void eraseAttr(const std::string &attrName);
  void eraseAttr(const std::string &name, const std::string &attrName);
  void eraseMask(const std::string &maskName);

  DatasetConstView slice(const Slice s) const &;
  DatasetView slice(const Slice s) &;
  Dataset slice(const Slice s) const &&;

  void rename(const Dim from, const Dim to);

  bool operator==(const Dataset &other) const;
  bool operator==(const DatasetConstView &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const DatasetConstView &other) const;

  Dataset &operator+=(const DataArrayConstView &other);
  Dataset &operator-=(const DataArrayConstView &other);
  Dataset &operator*=(const DataArrayConstView &other);
  Dataset &operator/=(const DataArrayConstView &other);
  Dataset &operator+=(const VariableConstView &other);
  Dataset &operator-=(const VariableConstView &other);
  Dataset &operator*=(const VariableConstView &other);
  Dataset &operator/=(const VariableConstView &other);
  Dataset &operator+=(const DatasetConstView &other);
  Dataset &operator-=(const DatasetConstView &other);
  Dataset &operator*=(const DatasetConstView &other);
  Dataset &operator/=(const DatasetConstView &other);

  std::unordered_map<Dim, scipp::index> dimensions() const;

private:
  friend class DatasetConstView;
  friend class DatasetView;
  friend class DataArrayConstView;
  friend class DataArrayView;
  friend class DataArray;

  void setData(const std::string &name, UnalignedData &&data);

  void setExtent(const Dim dim, const scipp::index extent, const bool isCoord);
  void setDims(const Dimensions &dims, const Dim coordDim = Dim::Invalid);
  void rebuildDims();

  template <class Key, class Val>
  void erase_from_map(std::unordered_map<Key, Val> &map, const Key &key) {
    map.erase(key);
    rebuildDims();
  }
  void setData_impl(const std::string &name, detail::DatasetData &&data,
                    const AttrPolicy attrPolicy);

  std::unordered_map<Dim, scipp::index> m_dims;
  std::unordered_map<Dim, Variable> m_coords;
  std::unordered_map<std::string, Variable> m_attrs;
  std::unordered_map<std::string, Variable> m_masks;
  detail::dataset_item_map m_data;
};

/// Const view for Dataset, implementing slicing and item selection.
class SCIPP_DATASET_EXPORT DatasetConstView {
  struct make_const_view {
    constexpr const DataArrayConstView &
    operator()(const DataArrayView &view) const noexcept {
      return view;
    }
  };

public:
  using value_type = Dataset;
  using key_type = std::string;
  using mapped_type = DataArray;

  DatasetConstView(const Dataset &dataset);

  static DatasetConstView makeViewWithEmptyIndexes(const Dataset &dataset) {
    auto res = DatasetConstView();
    res.m_dataset = &dataset;
    return res;
  }

  index size() const noexcept { return m_items.size(); }
  [[nodiscard]] bool empty() const noexcept { return m_items.empty(); }

  CoordsConstView coords() const noexcept;
  AttrsConstView attrs() const noexcept;
  MasksConstView masks() const noexcept;

  bool contains(const std::string &name) const noexcept;

  const DataArrayConstView &operator[](const std::string &name) const;

  auto begin() const && = delete;
  auto begin() const &noexcept {
    return boost::make_transform_iterator(m_items.begin(), make_const_view{});
  }
  auto end() const && = delete;
  auto end() const &noexcept {
    return boost::make_transform_iterator(m_items.end(), make_const_view{});
  }

  auto items_begin() const && = delete;
  auto items_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_key_value);
  }
  auto items_end() const && = delete;
  auto items_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_key_value);
  }

  auto keys_begin() const && = delete;
  /// Return const iterator to the beginning of all keys.
  auto keys_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_key);
  }
  auto keys_end() const && = delete;
  /// Return const iterator to the end of all keys.
  auto keys_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_key);
  }

  auto find(const std::string &name) const && = delete;
  auto find(const std::string &name) const &noexcept {
    return std::find_if(begin(), end(), [&name](const auto &item) {
      return item.name() == name;
    });
  }

  DatasetConstView slice(const Slice s) const;

  const auto &slices() const noexcept { return m_slices; }
  const auto &dataset() const noexcept { return *m_dataset; }

  bool operator==(const Dataset &other) const;
  bool operator==(const DatasetConstView &other) const;
  bool operator!=(const Dataset &other) const;
  bool operator!=(const DatasetConstView &other) const;
  std::unordered_map<Dim, scipp::index> dimensions() const;

protected:
  explicit DatasetConstView() : m_dataset(nullptr) {}
  template <class T>
  static std::pair<boost::container::small_vector<DataArrayView, 8>,
                   detail::slice_list>
  slice_items(const T &view, const Slice slice);
  const Dataset *m_dataset;
  boost::container::small_vector<DataArrayView, 8> m_items;
  void expectValidKey(const std::string &name) const;
  detail::slice_list m_slices;
};

/// View for Dataset, implementing slicing and item selection.
class SCIPP_DATASET_EXPORT DatasetView : public DatasetConstView {
  explicit DatasetView() : DatasetConstView(), m_mutableDataset(nullptr) {}

public:
  DatasetView(Dataset &dataset);

  CoordsView coords() const noexcept;
  AttrsView attrs() const noexcept;
  MasksView masks() const noexcept;

  const DataArrayView &operator[](const std::string &name) const;

  auto begin() const && = delete;
  auto begin() const &noexcept { return m_items.begin(); }
  auto end() const && = delete;
  auto end() const &noexcept { return m_items.end(); }

  auto items_begin() const && = delete;
  auto items_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_key_value);
  }
  auto items_end() const && = delete;
  auto items_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_key_value);
  }

  auto find(const std::string &name) const && = delete;
  auto find(const std::string &name) const &noexcept {
    return std::find_if(begin(), end(), [&name](const auto &item) {
      return item.name() == name;
    });
  }

  DatasetView slice(const Slice s) const;

  DatasetView operator+=(const DataArrayConstView &other) const;
  DatasetView operator-=(const DataArrayConstView &other) const;
  DatasetView operator*=(const DataArrayConstView &other) const;
  DatasetView operator/=(const DataArrayConstView &other) const;
  DatasetView operator+=(const VariableConstView &other) const;
  DatasetView operator-=(const VariableConstView &other) const;
  DatasetView operator*=(const VariableConstView &other) const;
  DatasetView operator/=(const VariableConstView &other) const;
  DatasetView operator+=(const DatasetConstView &other) const;
  DatasetView operator-=(const DatasetConstView &other) const;
  DatasetView operator*=(const DatasetConstView &other) const;
  DatasetView operator/=(const DatasetConstView &other) const;

  DatasetView assign(const DatasetConstView &other) const;

  auto &dataset() const noexcept { return *m_mutableDataset; }

private:
  Dataset *m_mutableDataset;
};

[[nodiscard]] SCIPP_DATASET_EXPORT DataArray
copy(const DataArrayConstView &array,
     const AttrPolicy attrPolicy = AttrPolicy::Keep);
[[nodiscard]] SCIPP_DATASET_EXPORT Dataset
copy(const DatasetConstView &dataset,
     const AttrPolicy attrPolicy = AttrPolicy::Keep);
SCIPP_DATASET_EXPORT DataArrayView
copy(const DataArrayConstView &array, const DataArrayView &out,
     const AttrPolicy attrPolicy = AttrPolicy::Keep);
SCIPP_DATASET_EXPORT DatasetView
copy(const DatasetConstView &dataset, const DatasetView &out,
     const AttrPolicy attrPolicy = AttrPolicy::Keep);

/// Data array, a variable with coordinates, masks, and attributes.
class SCIPP_DATASET_EXPORT DataArray {
public:
  using const_view_type = DataArrayConstView;
  using view_type = DataArrayView;

  DataArray() = default;
  explicit DataArray(const DataArrayConstView &view,
                     const AttrPolicy attrPolicy = AttrPolicy::Keep);

  template <class Data, class CoordMap = std::map<Dim, Variable>,
            class MasksMap = std::map<std::string, Variable>,
            class AttrMap = std::map<std::string, Variable>,
            typename = std::enable_if_t<std::is_same_v<Data, Variable> ||
                                        std::is_same_v<Data, UnalignedData>>>
  DataArray(Data data, CoordMap coords = {}, MasksMap masks = {},
            AttrMap attrs = {}, const std::string &name = "") {
    if (!data)
      throw std::runtime_error(
          "DataArray cannot be created with invalid content.");
    m_holder.setData(name, std::move(data));

    for (auto &&[dim, c] : coords)
      m_holder.setCoord(dim, std::move(c));

    for (auto &&[mask_name, m] : masks)
      m_holder.setMask(std::string(mask_name), std::move(m));

    for (auto &&[attr_name, a] : attrs)
      m_holder.setAttr(name, std::string(attr_name), std::move(a));
  }

  explicit operator bool() const noexcept { return !m_holder.empty(); }
  operator DataArrayConstView() const;
  operator DataArrayView();

  const std::string &name() const { return m_holder.begin()->name(); }
  void setName(const std::string &name);

  CoordsConstView coords() const;
  CoordsView coords();

  AttrsConstView attrs() const;
  AttrsView attrs();

  MasksConstView masks() const;
  MasksView masks();

  Dimensions dims() const { return get().dims(); }
  DType dtype() const { return get().dtype(); }
  units::Unit unit() const { return get().unit(); }

  DataArrayConstView unaligned() const { return get().unaligned(); }
  DataArrayView unaligned() { return get().unaligned(); }

  void setUnit(const units::Unit unit) { get().setUnit(unit); }

  /// Return true if the data array contains data values.
  bool hasData() const { return get().hasData(); }
  /// Return true if the data array contains data variances.
  bool hasVariances() const { return get().hasVariances(); }

  /// Return untyped const view for data (values and optional variances).
  VariableConstView data() const { return get().data(); }
  /// Return untyped view for data (values and optional variances).
  VariableView data() { return get().data(); }

  /// Return typed const view for data values.
  template <class T> auto values() const { return get().values<T>(); }
  /// Return typed view for data values.
  template <class T> auto values() { return get().values<T>(); }

  /// Return typed const view for data variances.
  template <class T> auto variances() const { return get().variances<T>(); }
  /// Return typed view for data variances.
  template <class T> auto variances() { return get().variances<T>(); }

  void rename(const Dim from, const Dim to) { m_holder.rename(from, to); }

  DataArray &operator+=(const DataArrayConstView &other);
  DataArray &operator-=(const DataArrayConstView &other);
  DataArray &operator*=(const DataArrayConstView &other);
  DataArray &operator/=(const DataArrayConstView &other);
  DataArray &operator+=(const VariableConstView &other);
  DataArray &operator-=(const VariableConstView &other);
  DataArray &operator*=(const VariableConstView &other);
  DataArray &operator/=(const VariableConstView &other);

  void setData(Variable data) {
    m_holder.setData(name(), std::move(data), AttrPolicy::Keep);
  }

  DataArrayConstView slice(const Slice s) const & { return get().slice(s); }
  DataArrayView slice(const Slice s) & { return get().slice(s); }
  DataArray slice(const Slice s) const && { return copy(get().slice(s)); }

  /// Iterable const view for generic code supporting Dataset and DataArray.
  DatasetConstView iterable_view() const noexcept { return m_holder; }
  /// Iterable view for generic code supporting Dataset and DataArray.
  DatasetView iterable_view() noexcept { return m_holder; }

  /// Return the Dataset holder of the given DataArray, so access to private
  /// members is possible, thus allowing moving of Variables without making
  /// copies.
  static Dataset to_dataset(DataArray &&data) {
    return std::move(data.m_holder);
  }

  void drop_alignment();

private:
  DataArrayConstView get() const;
  DataArrayView get();

  Dataset m_holder;
};

struct UnalignedData {
  explicit operator bool() const noexcept { return static_cast<bool>(data); }
  Dimensions dims;
  DataArray data;
};

SCIPP_DATASET_EXPORT DataArray operator+(const DataArrayConstView &a,
                                         const DataArrayConstView &b);
SCIPP_DATASET_EXPORT DataArray operator-(const DataArrayConstView &a,
                                         const DataArrayConstView &b);
SCIPP_DATASET_EXPORT DataArray operator*(const DataArrayConstView &a,
                                         const DataArrayConstView &b);
SCIPP_DATASET_EXPORT DataArray operator/(const DataArrayConstView &a,
                                         const DataArrayConstView &b);

SCIPP_DATASET_EXPORT DataArray operator+(const DataArrayConstView &a,
                                         const VariableConstView &b);
SCIPP_DATASET_EXPORT DataArray operator-(const DataArrayConstView &a,
                                         const VariableConstView &b);
SCIPP_DATASET_EXPORT DataArray operator*(const DataArrayConstView &a,
                                         const VariableConstView &b);
SCIPP_DATASET_EXPORT DataArray operator/(const DataArrayConstView &a,
                                         const VariableConstView &b);

SCIPP_DATASET_EXPORT DataArray operator+(const VariableConstView &a,
                                         const DataArrayConstView &b);
SCIPP_DATASET_EXPORT DataArray operator-(const VariableConstView &a,
                                         const DataArrayConstView &b);
SCIPP_DATASET_EXPORT DataArray operator*(const VariableConstView &a,
                                         const DataArrayConstView &b);
SCIPP_DATASET_EXPORT DataArray operator/(const VariableConstView &a,
                                         const DataArrayConstView &b);

SCIPP_DATASET_EXPORT Dataset operator+(const DatasetConstView &lhs,
                                       const DatasetConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator+(const DatasetConstView &lhs,
                                       const DataArrayConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator+(const DataArrayConstView &lhs,
                                       const DatasetConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator+(const DatasetConstView &lhs,
                                       const VariableConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator+(const VariableConstView &lhs,
                                       const DatasetConstView &rhs);

SCIPP_DATASET_EXPORT Dataset operator-(const DatasetConstView &lhs,
                                       const DatasetConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator-(const DatasetConstView &lhs,
                                       const DataArrayConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator-(const DataArrayConstView &lhs,
                                       const DatasetConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator-(const DatasetConstView &lhs,
                                       const VariableConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator-(const VariableConstView &lhs,
                                       const DatasetConstView &rhs);

SCIPP_DATASET_EXPORT Dataset operator*(const DatasetConstView &lhs,
                                       const DatasetConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator*(const DatasetConstView &lhs,
                                       const DataArrayConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator*(const DataArrayConstView &lhs,
                                       const DatasetConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator*(const DatasetConstView &lhs,
                                       const VariableConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator*(const VariableConstView &lhs,
                                       const DatasetConstView &rhs);

SCIPP_DATASET_EXPORT Dataset operator/(const DatasetConstView &lhs,
                                       const DatasetConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator/(const DatasetConstView &lhs,
                                       const DataArrayConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator/(const DataArrayConstView &lhs,
                                       const DatasetConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator/(const DatasetConstView &lhs,
                                       const VariableConstView &rhs);
SCIPP_DATASET_EXPORT Dataset operator/(const VariableConstView &lhs,
                                       const DatasetConstView &rhs);

SCIPP_DATASET_EXPORT DataArray astype(const DataArrayConstView &var,
                                      const DType type);

SCIPP_DATASET_EXPORT Dataset merge(const DatasetConstView &a,
                                   const DatasetConstView &b);

/// Return one of the inputs if they are the same, throw otherwise.
template <class T> T same(const T &a, const T &b) {
  core::expect::equals(a, b);
  return a;
}

/// Union the masks of the two proxies.
/// If any of the masks repeat they are OR'ed.
/// The result is stored in a new map
SCIPP_DATASET_EXPORT std::map<typename MasksConstView::key_type,
                              typename MasksConstView::mapped_type>
union_or(const MasksConstView &currentMasks, const MasksConstView &otherMasks);

/// Union the masks of the two proxies.
/// If any of the masks repeat they are OR'ed.
/// The result is stored in the first view.
SCIPP_DATASET_EXPORT void union_or_in_place(const MasksView &currentMasks,
                                            const MasksConstView &otherMasks);

SCIPP_DATASET_EXPORT bool
contains_events(const dataset::DataArrayConstView &array);

} // namespace scipp::dataset

namespace scipp {
using dataset::DataArray;
using dataset::DataArrayConstView;
using dataset::DataArrayView;
using dataset::Dataset;
using dataset::DatasetConstView;
using dataset::DatasetView;
} // namespace scipp
