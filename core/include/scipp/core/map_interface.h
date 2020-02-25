// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_MAP_INTERFACE_H
#define SCIPP_CORE_MAP_INTERFACE_H

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/variable.h"

namespace scipp::core {
namespace next {
namespace detail {
using slice_list =
    boost::container::small_vector<std::pair<Slice, scipp::index>, 2>;

template <class Var> auto makeSlice(Var &var, const slice_list &slices) {
  std::conditional_t<std::is_const_v<Var>, typename Var::const_view_type,
                     typename Var::view_type>
      slice(var);
  for (const auto [params, extent] : slices) {
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

static constexpr auto make_key_value = [](auto &&view) {
  using In = decltype(view);
  using View =
      std::conditional_t<std::is_rvalue_reference_v<In>, std::decay_t<In>, In>;
  return std::pair<const std::string &, View>(
      view.name(), std::forward<decltype(view)>(view));
};

static constexpr auto make_key = [](auto &&view) -> decltype(auto) {
  using T = std::decay_t<decltype(view)>;
  if constexpr (std::is_base_of_v<DataArrayConstView, T>)
    return view.name();
  else
    return view.first;
};

static constexpr auto make_value = [](auto &&view) -> decltype(auto) {
  return view.second;
};

} // namespace detail

// Use for CoordsConstView and UnalignedConstView
// former maps to Axis
// latter maps to Variable
template <class Derived, class Key, class Mapped> struct MapConstInterface {
  using key_type = Key;
  using mapped_type = Mapped;

  struct make_item {
    const Derived *view;
    template <class Item> auto operator()(const Item &item) const {
      return std::pair<key_type, typename mapped_type::const_view_type>(
          item.first, detail::makeSlice(*item.second.first, view->slices()));
    }
  };

  const auto &items() const noexcept {
    return static_cast<const Derived *>(this)->m_items;
  }
  const auto &slices() const noexcept {
    return static_cast<const Derived *>(this)->m_slices;
  }

  /// Return the number of coordinates in the view.
  index size() const noexcept { return scipp::size(items()); }
  /// Return true if there are 0 coordinates in the view.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  /// Returns whether a given key is present in the view.
  bool contains(const key_type &k) const {
    return items().find(k) != items().cend();
  }

  /// Return a const view to the coordinate for given dimension.
  typename mapped_type::const_view_type operator[](const key_type key) const {
    expect::contains(*this, key);
    return detail::makeSlice(*items().at(key).first, slices());
  }

  auto find(const key_type k) const && = delete;
  auto find(const key_type k) const &noexcept {
    return boost::make_transform_iterator(
        items().find(k), make_item{static_cast<const Derived *>(this)});
  }

  auto begin() const && = delete;
  /// Return const iterator to the beginning of all items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(
        items().begin(), make_item{static_cast<const Derived *>(this)});
  }
  auto end() const && = delete;
  /// Return const iterator to the end of all items.
  auto end() const &noexcept {
    return boost::make_transform_iterator(
        items().end(), make_item{static_cast<const Derived *>(this)});
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

template <class Derived, class Key, class Mapped> struct MapInterface {
  using key_type = Key;
  using mapped_type = Mapped;

  struct make_item {
    const Derived *view;
    template <class T> auto operator()(const T &item) const {
      return std::pair<key_type, typename mapped_type::view_type>(
          item.first, detail::makeSlice(*item.second.second, view->slices()));
    }
  };

  const auto &derived() const noexcept {
    return *static_cast<const Derived *>(this);
  }

  /// Return a view to the coordinate for given dimension.
  typename mapped_type::view_type operator[](const key_type key) const {
    // TODO
    // expect::contains(derived(), key);
    return detail::makeSlice(*derived().items().at(key).second,
                             static_cast<const Derived *>(this)->slices());
  }

  template <class T> auto find(const T k) const && = delete;
  template <class T> auto find(const T k) const &noexcept {
    return boost::make_transform_iterator(derived().items().find(k),
                                          make_item{this});
  }

  auto begin() const && = delete;
  /// Return iterator to the beginning of all items.
  auto begin() const &noexcept {
    return boost::make_transform_iterator(derived().items().begin(),
                                          make_item{this});
  }
  auto end() const && = delete;
  /// Return iterator to the end of all items.
  auto end() const &noexcept {
    return boost::make_transform_iterator(derived().items().end(),
                                          make_item{this});
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
};

#if 0
class UnalignedConstView
    : MapConstInterface<UnalignedConstView, Dim, Variable> {
public:
  using key_type = Dim;
  using mapped_type = Variable;

  UnalignedConstView(const std::unordered_map<std::string, Variable> &items) {
    for (const auto &[key, value] : items)
      m_items.emplace(key, std::pair{&value, nullptr});
  }

private:
  std::unordered_map<key_type, std::pair<const mapped_type *, mapped_type *>>
      m_items;
  detail::slice_list m_slices;
};

class UnalignedView : public UnalignedConstView,
    : MapInterface<UnalignedView, Dim, Variable> {
public:
  using key_type = Dim;
  using mapped_type = Variable;

  UnalignedView(const std::unordered_map<std::string, Variable> &items) {
    for (const auto &[key, value] : items)
      m_items.emplace(key, std::pair{&value, nullptr});
  }
};
#endif

} // namespace next
} // namespace scipp::core
#endif // SCIPP_CORE_MAP_INTERFACE_H
