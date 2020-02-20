// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_DATASET_H
#define SCIPP_DATASET_H

#include <functional>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/except.h"
#include "scipp/core/variable.h"
#include "scipp/core/view_decl.h"

namespace scipp::core {

class DataArray;
class Dataset;
class DatasetConstView;
class DatasetView;

namespace detail {
/// Helper for holding data items in Dataset.
struct DatasetData {
  /// Optional data values (with optional variances).
  std::optional<Variable> data;
  /// Dimension coord for the sparse dimension (there can be only 1).
  std::optional<Variable> coord;
  /// Potential labels for the sparse dimension.
  std::unordered_map<std::string, Variable> labels;
  /// Attributes for data.
  std::unordered_map<std::string, Variable> attrs;
};

using dataset_item_map = std::unordered_map<std::string, DatasetData>;
} // namespace detail

/// Const view for a data item and related coordinates of Dataset.
class SCIPP_CORE_EXPORT DataArrayConstView {
public:
  DataArrayConstView(const Dataset &dataset,
                     const detail::dataset_item_map::value_type &data,
                     const detail::slice_list &slices = {},
                     std::optional<VariableView> &&view = std::nullopt);

  const std::string &name() const noexcept;

  Dimensions dims() const noexcept;
  DType dtype() const;
  units::Unit unit() const;

  CoordsConstView coords() const noexcept;
  LabelsConstView labels() const noexcept;
  AttrsConstView attrs() const noexcept;
  MasksConstView masks() const noexcept;

  /// Return true if the view contains data values.
  bool hasData() const noexcept { return m_data->second.data.has_value(); }
  /// Return true if the view contains data variances.
  bool hasVariances() const noexcept {
    return hasData() && m_data->second.data->hasVariances();
  }

  /// Return untyped const view for data (values and optional variances).
  const VariableConstView &data() const {
    if (!hasData())
      throw except::SparseDataError("No data in item.");
    return *m_view;
  }
  /// Return typed const view for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed const view for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  DataArrayConstView slice(const Slice slice1) const;
  DataArrayConstView slice(const Slice slice1, const Slice slice2) const;
  DataArrayConstView slice(const Slice slice1, const Slice slice2,
                           const Slice slice3) const;

  const detail::slice_list &slices() const noexcept { return m_slices; }

  auto &underlying() const { return m_data->second; }

protected:
  // Note that m_view is a VariableView, not a VariableConstView. In case
  // *this (DataArrayConstView) is stand-alone (not part of DataArrayView),
  // m_view is actually just a VariableConstView wrapped in an (invalid)
  // VariableView. The interface guarantees that the invalid mutable view is
  // not accessible. This wrapping avoids inefficient duplication of the view in
  // the child class DataArrayView.
  std::optional<VariableView> m_view;

private:
  friend class DatasetConstView;
  friend class DatasetView;

  const Dataset *m_dataset;
  const detail::dataset_item_map::value_type *m_data;
  detail::slice_list m_slices;
};

SCIPP_CORE_EXPORT bool operator==(const DataArrayConstView &a,
                                  const DataArrayConstView &b);
SCIPP_CORE_EXPORT bool operator!=(const DataArrayConstView &a,
                                  const DataArrayConstView &b);

class DatasetConstView;
class DatasetView;
class Dataset;

/// View for a data item and related coordinates of Dataset.
class SCIPP_CORE_EXPORT DataArrayView : public DataArrayConstView {
public:
  DataArrayView(Dataset &dataset, detail::dataset_item_map::value_type &data,
                const detail::slice_list &slices = {});

  CoordsView coords() const noexcept;
  LabelsView labels() const noexcept;
  MasksView masks() const noexcept;
  AttrsView attrs() const noexcept;

  void setUnit(const units::Unit unit) const;

  /// Return untyped view for data (values and optional variances).
  const VariableView &data() const {
    if (!hasData())
      throw except::SparseDataError("No data in item.");
    return *m_view;
  }
  /// Return typed view for data values.
  template <class T> auto values() const { return data().template values<T>(); }

  /// Return typed view for data variances.
  template <class T> auto variances() const {
    return data().template variances<T>();
  }

  DataArrayView slice(const Slice slice1) const;
  DataArrayView slice(const Slice slice1, const Slice slice2) const;
  DataArrayView slice(const Slice slice1, const Slice slice2,
                      const Slice slice3) const;

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

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DataArrayView operator+=(const T value) const {
    return *this += makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DataArrayView operator-=(const T value) const {
    return *this -= makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DataArrayView operator*=(const T value) const {
    return *this *= makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DataArrayView operator/=(const T value) const {
    return *this /= makeVariable<T>(Values{value});
  }

private:
  friend class DatasetConstView;
  // For internal use in DatasetConstView.
  explicit DataArrayView(DataArrayConstView &&base)
      : DataArrayConstView(std::move(base)), m_mutableDataset{nullptr},
        m_mutableData{nullptr} {}

  Dataset *m_mutableDataset;
  detail::dataset_item_map::value_type *m_mutableData;
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
template <class D> make_item(D *)->make_item<D>;

} // namespace detail

/// Collection of data arrays.
class SCIPP_CORE_EXPORT Dataset {
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

  template <class DataMap, class CoordMap, class LabelsMap, class MasksMap,
            class AttrMap>
  Dataset(DataMap data, CoordMap coords, LabelsMap labels, MasksMap masks,
          AttrMap attrs) {
    for (auto &&[dim, coord] : coords)
      setCoord(dim, std::move(coord));
    for (auto &&[name, labs] : labels)
      setLabels(std::string(name), std::move(labs));
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
  /// and sparse coordinates).
  index size() const noexcept { return scipp::size(m_data); }
  /// Return true if there are 0 data items in the dataset.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  void clear();

  CoordsConstView coords() const noexcept;
  CoordsView coords() noexcept;

  LabelsConstView labels() const noexcept;
  LabelsView labels() noexcept;

  AttrsConstView attrs() const noexcept;
  AttrsView attrs() noexcept;

  MasksConstView masks() const noexcept;
  MasksView masks() noexcept;

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

  auto items_begin() const && = delete;
  auto items_begin() && = delete;
  auto items_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_key_value);
  }
  auto items_begin() & noexcept {
    return boost::make_transform_iterator(begin(), detail::make_key_value);
  }
  auto items_end() const && = delete;
  auto items_end() && = delete;
  auto items_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_key_value);
  }

  auto items_end() & noexcept {
    return boost::make_transform_iterator(end(), detail::make_key_value);
  }

  auto keys_begin() const && = delete;
  auto keys_begin() && = delete;
  auto keys_begin() const &noexcept {
    return boost::make_transform_iterator(m_data.begin(), detail::make_key);
  }
  auto keys_begin() & noexcept {
    return boost::make_transform_iterator(m_data.begin(), detail::make_key);
  }
  auto keys_end() const && = delete;
  auto keys_end() && = delete;
  auto keys_end() const &noexcept {
    return boost::make_transform_iterator(m_data.end(), detail::make_key);
  }

  auto keys_end() & noexcept {
    return boost::make_transform_iterator(m_data.end(), detail::make_key);
  }

  void setCoord(const Dim dim, Variable coord);
  void setLabels(const std::string &labelName, Variable labels);
  void setMask(const std::string &masksName, Variable masks);
  void setAttr(const std::string &attrName, Variable attr);
  void setAttr(const std::string &name, const std::string &attrName,
               Variable attr);
  void setData(const std::string &name, Variable data);
  void setData(const std::string &name, const DataArrayConstView &data);
  void setData(const std::string &name, DataArray data);
  void setSparseCoord(const std::string &name, Variable coord);
  void setSparseLabels(const std::string &name, const std::string &labelName,
                       Variable labels);

  void setCoord(const Dim dim, const VariableConstView &coord) {
    setCoord(dim, Variable(coord));
  }
  void setLabels(const std::string &labelName,
                 const VariableConstView &labels) {
    setLabels(labelName, Variable(labels));
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
  void setData(const std::string &name, const VariableConstView &data) {
    setData(name, Variable(data));
  }
  void setSparseCoord(const std::string &name, const VariableConstView &coord) {
    setSparseCoord(name, Variable(coord));
  }
  void setSparseLabels(const std::string &name, const std::string &labelName,
                       const VariableConstView &labels) {
    setSparseLabels(name, labelName, Variable(labels));
  }

  void eraseCoord(const Dim dim);
  void eraseLabels(const std::string &labelName);
  void eraseAttr(const std::string &attrName);
  void eraseAttr(const std::string &name, const std::string &attrName);
  void eraseMask(const std::string &maskName);
  void eraseSparseCoord(const std::string &name);
  void eraseSparseLabels(const std::string &name, const std::string &labelName);

  DatasetConstView slice(const Slice slice1) const &;
  DatasetConstView slice(const Slice slice1, const Slice slice2) const &;
  DatasetConstView slice(const Slice slice1, const Slice slice2,
                         const Slice slice3) const &;
  DatasetView slice(const Slice slice1) &;
  DatasetView slice(const Slice slice1, const Slice slice2) &;
  DatasetView slice(const Slice slice1, const Slice slice2,
                    const Slice slice3) &;
  Dataset slice(const Slice slice1) const &&;
  Dataset slice(const Slice slice1, const Slice slice2) const &&;
  Dataset slice(const Slice slice1, const Slice slice2,
                const Slice slice3) const &&;

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
  Dataset &operator+=(const Dataset &other);
  Dataset &operator-=(const Dataset &other);
  Dataset &operator*=(const Dataset &other);
  Dataset &operator/=(const Dataset &other);

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  Dataset &operator+=(const T value) {
    return *this += makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  Dataset &operator-=(const T value) {
    return *this -= makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  Dataset &operator*=(const T value) {
    return *this *= makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  Dataset &operator/=(const T value) {
    return *this /= makeVariable<T>(Values{value});
  }

  std::unordered_map<Dim, scipp::index> dimensions() const;

private:
  friend class DatasetConstView;
  friend class DatasetView;
  friend class DataArrayConstView;
  friend class DataArrayView;

  void setExtent(const Dim dim, const scipp::index extent, const bool isCoord);
  void setDims(const Dimensions &dims, const Dim coordDim = Dim::Invalid);
  void rebuildDims();

  template <class Key, class Val>
  void erase_from_map(std::unordered_map<Key, Val> &map, const Key &key) {
    map.erase(key);
    rebuildDims();
  }

  std::unordered_map<Dim, scipp::index> m_dims;
  std::unordered_map<Dim, Variable> m_coords;
  std::unordered_map<std::string, Variable> m_labels;
  std::unordered_map<std::string, Variable> m_attrs;
  std::unordered_map<std::string, Variable> m_masks;
  detail::dataset_item_map m_data;
};

/// Common functionality for other view classes.
template <class Base> class MutableView : public Base {
private:
  struct make_item {
    const MutableView<Base> *view;
    template <class T> auto operator()(const T &item) const {
      return std::pair<typename Base::key_type, VariableView>(
          item.first, detail::makeSlice(*item.second.second, view->slices()));
    }
  };

  MutableView(Dataset *parent, const std::string *name, Base &&base)
      : Base(std::move(base)), m_parent(parent), m_name(name) {}

  Dataset *m_parent;
  const std::string *m_name;

public:
  MutableView(
      Dataset *parent, const std::string *name,
      std::unordered_map<typename Base::key_type,
                         std::pair<const Variable *, Variable *>> &&items,
      const detail::slice_list &slices = {})
      : Base(std::move(items), slices), m_parent(parent), m_name(name) {}

  /// Return a view to the coordinate for given dimension.
  VariableView operator[](const typename Base::key_type key) const {
    expect::contains(*this, key);
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

  auto items_begin() const && = delete;
  /// Return iterator to the beginning of all items.
  auto items_begin() const &noexcept { return begin(); }
  auto items_end() const && = delete;
  /// Return iterator to the end of all items.
  auto items_end() const &noexcept { return end(); }

  auto values_begin() const && = delete;
  /// Return iterator to the beginning of all values.
  auto values_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_value);
  }
  auto values_end() const && = delete;
  /// Return iterator to the end of all values.
  auto values_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_value);
  }

  MutableView slice(const Slice slice1) const {
    // parent = nullptr since adding coords via slice is not supported.
    return MutableView(nullptr, m_name, Base::slice(slice1));
  }

  MutableView slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  MutableView slice(const Slice slice1, const Slice slice2,
                    const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

  template <class VarOrView>
  void set(const typename Base::key_type key, VarOrView var) const {
    if (!m_parent || !Base::m_slices.empty())
      throw std::runtime_error(
          "Cannot add coord/labels/attr field to a slice.");
    if (var.dims().sparse()) {
      if (!m_name)
        throw std::runtime_error("Sparse coord/labels/attr must be added to "
                                 "coords of dataset items, not coords of "
                                 "dataset.");
      if constexpr (std::is_same_v<Base, CoordsConstView>)
        m_parent->setSparseCoord(*m_name, std::move(var));
      if constexpr (std::is_same_v<Base, LabelsConstView>)
        m_parent->setSparseLabels(*m_name, key, std::move(var));
      if constexpr (std::is_same_v<Base, AttrsConstView>)
        throw std::runtime_error("Attributes cannot be sparse.");
    } else {
      if constexpr (std::is_same_v<Base, CoordsConstView>)
        m_parent->setCoord(key, std::move(var));
      if constexpr (std::is_same_v<Base, LabelsConstView>)
        m_parent->setLabels(key, std::move(var));
      if constexpr (std::is_same_v<Base, MasksConstView>)
        m_parent->setMask(key, std::move(var));
      if constexpr (std::is_same_v<Base, AttrsConstView>) {
        if (m_name)
          m_parent->setAttr(*m_name, key, std::move(var));
        else
          m_parent->setAttr(key, std::move(var));
      }
    }
    // TODO rebuild *this?!
  }

  void erase(const typename Base::key_type key) {
    if (!m_parent || !Base::m_slices.empty())
      throw std::runtime_error(
          "Cannot remove coord/labels/attr field from a slice.");

    bool sparse = m_name; // Does view point on sparse data or not
    if (sparse)
      sparse &= (*m_parent)[*m_name].dims().sparse();

    if (!sparse) {
      if constexpr (std::is_same_v<Base, CoordsConstView>)
        m_parent->eraseCoord(key);
      if constexpr (std::is_same_v<Base, LabelsConstView>)
        m_parent->eraseLabels(key);
      if constexpr (std::is_same_v<Base, AttrsConstView>) {
        if (m_name)
          m_parent->eraseAttr(*m_name, key);
        else
          m_parent->eraseAttr(key);
      }
      if constexpr (std::is_same_v<Base, MasksConstView>)
        m_parent->eraseMask(key);
    } else {
      if constexpr (std::is_same_v<Base, CoordsConstView>) {
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
      } else if constexpr (std::is_same_v<Base, LabelsConstView>) {
        if (this->operator[](key).dims().sparse())
          m_parent->eraseSparseLabels(*m_name, key);
        else
          m_parent->eraseLabels(key);
      } else if constexpr (std::is_same_v<Base, AttrsConstView>)
        m_parent->eraseAttr(*m_name, key);
      else
        throw std::runtime_error("The instance cannot be sparse.");
    }
  }
};

template <class T1, class T2> auto union_(const T1 &a, const T2 &b) {
  std::map<typename T1::key_type, typename T1::mapped_type> out;

  for (const auto &[key, item] : a)
    out.emplace(key, item);
  for (const auto &[key, item] : b) {
    if (const auto it = a.find(key); it != a.end())
      expect::equals(item, it->second);
    else
      out.emplace(key, item);
  }
  return out;
}

/// Const view for Dataset, implementing slicing and item selection.
class SCIPP_CORE_EXPORT DatasetConstView {
  struct make_const_view {
    constexpr const DataArrayConstView &
    operator()(const DataArrayView &view) const noexcept {
      return view;
    }
  };

public:
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
  LabelsConstView labels() const noexcept;
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

  DatasetConstView slice(const Slice slice1) const;

  DatasetConstView slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  DatasetConstView slice(const Slice slice1, const Slice slice2,
                         const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

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
class SCIPP_CORE_EXPORT DatasetView : public DatasetConstView {
  explicit DatasetView() : DatasetConstView(), m_mutableDataset(nullptr) {}

public:
  DatasetView(Dataset &dataset);

  CoordsView coords() const noexcept;
  LabelsView labels() const noexcept;
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

  DatasetView slice(const Slice slice1) const;

  DatasetView slice(const Slice slice1, const Slice slice2) const {
    return slice(slice1).slice(slice2);
  }

  DatasetView slice(const Slice slice1, const Slice slice2,
                    const Slice slice3) const {
    return slice(slice1, slice2).slice(slice3);
  }

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
  DatasetView operator+=(const Dataset &other) const;
  DatasetView operator-=(const Dataset &other) const;
  DatasetView operator*=(const Dataset &other) const;
  DatasetView operator/=(const Dataset &other) const;

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DatasetView operator+=(const T value) const {
    return *this += makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DatasetView operator-=(const T value) const {
    return *this -= makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DatasetView operator*=(const T value) const {
    return *this *= makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DatasetView operator/=(const T value) const {
    return *this /= makeVariable<T>(Values{value});
  }

  DatasetView assign(const DatasetConstView &other) const;

  auto &dataset() const noexcept { return *m_mutableDataset; }

private:
  Dataset *m_mutableDataset;
};

SCIPP_CORE_EXPORT DataArray copy(const DataArrayConstView &array);
SCIPP_CORE_EXPORT Dataset copy(const DatasetConstView &dataset);

/// Data array, a variable with coordinates, labels, and attributes.
class SCIPP_CORE_EXPORT DataArray {
public:
  using const_view_type = DataArrayConstView;
  using view_type = DataArrayView;

  DataArray() = default;
  explicit DataArray(const DataArrayConstView &view);
  template <class CoordMap = std::map<Dim, Variable>,
            class LabelsMap = std::map<std::string, Variable>,
            class MasksMap = std::map<std::string, Variable>,
            class AttrMap = std::map<std::string, Variable>>
  DataArray(std::optional<Variable> data, CoordMap coords = {},
            LabelsMap labels = {}, MasksMap masks = {}, AttrMap attrs = {},
            const std::string &name = "") {
    if (data)
      m_holder.setData(name, std::move(*data));

    for (auto &&[dim, c] : coords)
      if (c.dims().sparse())
        m_holder.setSparseCoord(name, std::move(c));
      else
        m_holder.setCoord(dim, std::move(c));

    for (auto &&[label_name, l] : labels)
      if (l.dims().sparse())
        m_holder.setSparseLabels(name, std::string(label_name), std::move(l));
      else
        m_holder.setLabels(std::string(label_name), std::move(l));

    for (auto &&[mask_name, m] : masks)
      m_holder.setMask(std::string(mask_name), std::move(m));

    for (auto &&[attr_name, a] : attrs)
      m_holder.setAttr(name, std::string(attr_name), std::move(a));

    if (m_holder.size() != 1)
      throw std::runtime_error(
          "DataArray must have either data or a sparse coordinate.");
  }

  explicit operator bool() const noexcept { return !m_holder.empty(); }
  operator DataArrayConstView() const;
  operator DataArrayView();

  const std::string &name() const { return m_holder.begin()->name(); }

  CoordsConstView coords() const { return get().coords(); }
  CoordsView coords() { return get().coords(); }

  LabelsConstView labels() const { return get().labels(); }
  LabelsView labels() { return get().labels(); }

  AttrsConstView attrs() const { return get().attrs(); }
  AttrsView attrs() { return get().attrs(); }

  MasksConstView masks() const { return get().masks(); }
  MasksView masks() { return get().masks(); }

  Dimensions dims() const { return get().dims(); }
  DType dtype() const { return get().dtype(); }
  units::Unit unit() const { return get().unit(); }

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

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DataArray &operator+=(const T value) {
    return *this += makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DataArray &operator-=(const T value) {
    return *this -= makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DataArray &operator*=(const T value) {
    return *this *= makeVariable<T>(Values{value});
  }

  template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
  DataArray &operator/=(const T value) {
    return *this /= makeVariable<T>(Values{value});
  }

  void setData(Variable data) { m_holder.setData(name(), std::move(data)); }

  // TODO need to define some details regarding handling of dense coords in case
  // the array is sparse, not exposing this to Python for now.
  void setCoord(const Dim dim, Variable coord) {
    m_holder.setCoord(dim, std::move(coord));
  }
  void setCoord(const Dim dim, const VariableConstView &coord) {
    setCoord(dim, Variable(coord));
  }

  DataArrayConstView slice(const Slice slice1) const & {
    return get().slice(slice1);
  }
  DataArrayConstView slice(const Slice slice1, const Slice slice2) const & {
    return get().slice(slice1, slice2);
  }
  DataArrayConstView slice(const Slice slice1, const Slice slice2,
                           const Slice slice3) const & {
    return get().slice(slice1, slice2, slice3);
  }
  DataArrayView slice(const Slice slice1) & { return get().slice(slice1); }
  DataArrayView slice(const Slice slice1, const Slice slice2) & {
    return get().slice(slice1, slice2);
  }
  DataArrayView slice(const Slice slice1, const Slice slice2,
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

private:
  DataArrayConstView get() const;
  DataArrayView get();

  Dataset m_holder;
};

SCIPP_CORE_EXPORT DataArray operator+(const DataArrayConstView &a,
                                      const DataArrayConstView &b);
SCIPP_CORE_EXPORT DataArray operator-(const DataArrayConstView &a,
                                      const DataArrayConstView &b);
SCIPP_CORE_EXPORT DataArray operator*(const DataArrayConstView &a,
                                      const DataArrayConstView &b);
SCIPP_CORE_EXPORT DataArray operator/(const DataArrayConstView &a,
                                      const DataArrayConstView &b);

SCIPP_CORE_EXPORT DataArray operator+(const DataArrayConstView &a,
                                      const VariableConstView &b);
SCIPP_CORE_EXPORT DataArray operator-(const DataArrayConstView &a,
                                      const VariableConstView &b);
SCIPP_CORE_EXPORT DataArray operator*(const DataArrayConstView &a,
                                      const VariableConstView &b);
SCIPP_CORE_EXPORT DataArray operator/(const DataArrayConstView &a,
                                      const VariableConstView &b);

SCIPP_CORE_EXPORT DataArray operator+(const VariableConstView &a,
                                      const DataArrayConstView &b);
SCIPP_CORE_EXPORT DataArray operator-(const VariableConstView &a,
                                      const DataArrayConstView &b);
SCIPP_CORE_EXPORT DataArray operator*(const VariableConstView &a,
                                      const DataArrayConstView &b);
SCIPP_CORE_EXPORT DataArray operator/(const VariableConstView &a,
                                      const DataArrayConstView &b);

SCIPP_CORE_EXPORT Dataset operator+(const Dataset &lhs, const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const Dataset &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const Dataset &lhs,
                                    const DataArrayConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DatasetConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DatasetConstView &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DatasetConstView &lhs,
                                    const DataArrayConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DataArrayConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DataArrayConstView &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const Dataset &lhs,
                                    const VariableConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const VariableConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const DatasetConstView &lhs,
                                    const VariableConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator+(const VariableConstView &lhs,
                                    const DatasetConstView &rhs);

SCIPP_CORE_EXPORT Dataset operator-(const Dataset &lhs, const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const Dataset &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const Dataset &lhs,
                                    const DataArrayConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DatasetConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DatasetConstView &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DatasetConstView &lhs,
                                    const DataArrayConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DataArrayConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DataArrayConstView &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const Dataset &lhs,
                                    const VariableConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const VariableConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const DatasetConstView &lhs,
                                    const VariableConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator-(const VariableConstView &lhs,
                                    const DatasetConstView &rhs);

SCIPP_CORE_EXPORT Dataset operator*(const Dataset &lhs, const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const Dataset &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const Dataset &lhs,
                                    const DataArrayConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DatasetConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DatasetConstView &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DatasetConstView &lhs,
                                    const DataArrayConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DataArrayConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DataArrayConstView &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const Dataset &lhs,
                                    const VariableConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const VariableConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const DatasetConstView &lhs,
                                    const VariableConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator*(const VariableConstView &lhs,
                                    const DatasetConstView &rhs);

SCIPP_CORE_EXPORT Dataset operator/(const Dataset &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const Dataset &lhs, const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const Dataset &lhs,
                                    const DataArrayConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DatasetConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DatasetConstView &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DatasetConstView &lhs,
                                    const DataArrayConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DataArrayConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DataArrayConstView &lhs,
                                    const DatasetConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const Dataset &lhs,
                                    const VariableConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const VariableConstView &lhs,
                                    const Dataset &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const DatasetConstView &lhs,
                                    const VariableConstView &rhs);
SCIPP_CORE_EXPORT Dataset operator/(const VariableConstView &lhs,
                                    const DatasetConstView &rhs);

template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Dataset operator+(const T value, const DatasetConstView &a) {
  return makeVariable<T>(Values{value}) + a;
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Dataset operator-(const T value, const DatasetConstView &a) {
  return makeVariable<T>(Values{value}) - a;
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Dataset operator*(const T value, const DatasetConstView &a) {
  return makeVariable<T>(Values{value}) * a;
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Dataset operator/(const T value, const DatasetConstView &a) {
  return makeVariable<T>(Values{value}) / a;
}

template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Dataset operator+(const DatasetConstView &a, const T value) {
  return a + makeVariable<T>(Values{value});
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Dataset operator-(const DatasetConstView &a, const T value) {
  return a - makeVariable<T>(Values{value});
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Dataset operator*(const DatasetConstView &a, const T value) {
  return a * makeVariable<T>(Values{value});
}
template <typename T, typename = std::enable_if_t<!is_container_or_view<T>()>>
Dataset operator/(const DatasetConstView &a, const T value) {
  return a / makeVariable<T>(Values{value});
}

SCIPP_CORE_EXPORT DataArray astype(const DataArrayConstView &var,
                                   const DType type);

SCIPP_CORE_EXPORT DataArray histogram(const DataArrayConstView &sparse,
                                      const Variable &binEdges);
SCIPP_CORE_EXPORT DataArray histogram(const DataArrayConstView &sparse,
                                      const VariableConstView &binEdges);
SCIPP_CORE_EXPORT Dataset histogram(const Dataset &dataset,
                                    const VariableConstView &bins);
SCIPP_CORE_EXPORT Dataset histogram(const Dataset &dataset,
                                    const Variable &bins);
SCIPP_CORE_EXPORT Dataset histogram(const Dataset &dataset, const Dim &dim);

SCIPP_CORE_EXPORT Dataset merge(const DatasetConstView &a,
                                const DatasetConstView &b);

SCIPP_CORE_EXPORT DataArray flatten(const DataArrayConstView &a, const Dim dim);
SCIPP_CORE_EXPORT Dataset flatten(const DatasetConstView &d, const Dim dim);

SCIPP_CORE_EXPORT DataArray sum(const DataArrayConstView &a, const Dim dim);
SCIPP_CORE_EXPORT Dataset sum(const DatasetConstView &d, const Dim dim);

SCIPP_CORE_EXPORT DataArray mean(const DataArrayConstView &a, const Dim dim);
SCIPP_CORE_EXPORT Dataset mean(const DatasetConstView &d, const Dim dim);

SCIPP_CORE_EXPORT DataArray concatenate(const DataArrayConstView &a,
                                        const DataArrayConstView &b,
                                        const Dim dim);
SCIPP_CORE_EXPORT Dataset concatenate(const DatasetConstView &a,
                                      const DatasetConstView &b, const Dim dim);

SCIPP_CORE_EXPORT DataArray rebin(const DataArrayConstView &a, const Dim dim,
                                  const VariableConstView &coord);
SCIPP_CORE_EXPORT Dataset rebin(const DatasetConstView &d, const Dim dim,
                                const VariableConstView &coord);

SCIPP_CORE_EXPORT DataArray resize(const DataArrayConstView &a, const Dim dim,
                                   const scipp::index size);
SCIPP_CORE_EXPORT Dataset resize(const DatasetConstView &d, const Dim dim,
                                 const scipp::index size);

[[nodiscard]] SCIPP_CORE_EXPORT DataArray
reciprocal(const DataArrayConstView &a);

SCIPP_CORE_EXPORT VariableConstView same(const VariableConstView &a,
                                         const VariableConstView &b);

/// Union the masks of the two proxies.
/// If any of the masks repeat they are OR'ed.
/// The result is stored in a new map
SCIPP_CORE_EXPORT std::map<typename MasksConstView::key_type,
                           typename MasksConstView::mapped_type>
union_or(const MasksConstView &currentMasks, const MasksConstView &otherMasks);

/// Union the masks of the two proxies.
/// If any of the masks repeat they are OR'ed.
/// The result is stored in the first view.
SCIPP_CORE_EXPORT void union_or_in_place(const MasksView &currentMasks,
                                         const MasksConstView &otherMasks);

} // namespace scipp::core

#endif // SCIPP_DATASET_H
