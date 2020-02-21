// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_MAP_INTERFACE_H
#define SCIPP_CORE_MAP_INTERFACE_H

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/variable.h"

namespace scipp::core {

// Use for CoordsConstView and UnalignedConstView
// former maps to Axis
// latter maps to Variable
template <class Derived> struct MapConstInterface {
  using key_type = Derived::m_items::key_type;
  using mapped_type = Derived::m_items::mapped_type;

  struct make_item {
    const ConstView *view;
    template <class Item> auto operator()(const Item &item) const {
      return std::pair<key_type, VariableConstView>(
          item.first, detail::makeSlice(*item.second.first, view->slices()));
    }
  };

  const auto &items() const { return static_cast<const T *>(this)->m_items; }

  /// Return the number of coordinates in the view.
  index size() const noexcept { return scipp::size(items()); }
  /// Return true if there are 0 coordinates in the view.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  /// Returns whether a given key is present in the view.
  bool contains(const key_type &k) const {
    return items().find(k) != items().cend();
  }

  /// Return a const view to the coordinate for given dimension.
  VariableConstView operator[](const key_type key) const {
    expect::contains(*this, key);
    return detail::makeSlice(*items().at(key).first, m_slices);
  }

  auto find(const key_type k) const && = delete;
  auto find(const key_type k) const &noexcept {
    return boost::make_transform_iterator(items().find(k), make_item{this});
  }

  auto begin() const && = delete;
  /// Return const iterator to the beginning of all items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(items().begin(), make_item{this});
  }
  auto end() const && = delete;
  /// Return const iterator to the end of all items.
  auto end() const &noexcept {
    return boost::make_transform_iterator(items().end(), make_item{this});
  }

  auto items_begin() const && = delete;
  /// Return const iterator to the beginning of all items.
  auto items_begin() const &noexcept { return begin(); }
  auto items_end() const && = delete;
  /// Return const iterator to the end of all items.
  auto items_end() const &noexcept { return end(); }

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

  auto values_begin() const && = delete;
  /// Return const iterator to the beginning of all values.
  auto values_begin() const &noexcept {
    return boost::make_transform_iterator(begin(), detail::make_value);
  }
  auto values_end() const && = delete;
  /// Return const iterator to the end of all values.
  auto values_end() const &noexcept {
    return boost::make_transform_iterator(end(), detail::make_value);
  }
};


} // namespace scipp::core
#endif // SCIPP_CORE_MAP_INTERFACE_H
