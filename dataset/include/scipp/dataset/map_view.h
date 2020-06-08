// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/slice.h"
#include "scipp/dataset/dataset_access.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/map_view_forward.h"
#include "scipp/units/dim.h"
#include "scipp/units/unit.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {

namespace detail {
using slice_list =
    boost::container::small_vector<std::pair<Slice, scipp::index>, 2>;

template <class T> void do_make_slice(T &slice, const slice_list &slices) {
  for (const auto &[params, extent] : slices) {
    if (slice.dims().contains(params.dim())) {
      if (slice.dims()[params.dim()] == extent) {
        slice = slice.slice(params);
      } else {
        slice = slice.slice(
            Slice{params.dim(), params.begin(),
                  params.end() == -1 ? params.begin() + 2 : params.end() + 1});
      }
    }
  }
}

template <class Var> auto makeSlice(Var &var, const slice_list &slices) {
  std::conditional_t<std::is_const_v<Var>, typename Var::const_view_type,
                     typename Var::view_type>
      slice(var);
  do_make_slice(slice, slices);
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

/// Return the dimension for given coord.
/// @param var Coordinate variable
/// @param key Key of the coordinate in a coord dict
///
/// For dimension-coords, this is the same as the key, for non-dimension-coords
/// (labels) we adopt the convention that they are "label" their inner
/// dimension. Returns Dim::Invalid for 0-D var or var containing event lists.
template <class T, class Key> Dim dim_of_coord(const T &var, const Key &key) {
  if (contains_events(var) || var.dims().ndim() == 0)
    return Dim::Invalid;
  if constexpr (std::is_same_v<Key, Dim>) {
    const bool is_dimension_coord = var.dims().contains(key);
    return is_dimension_coord ? key : var.dims().inner();
  } else
    return var.dims().inner();
}

/// Common functionality for other const-view classes.
template <class Id, class Key, class Value> class ConstView {
public:
  using key_type = Key;
  using mapped_type = Value;
  using holder_type =
      std::unordered_map<key_type, typename mapped_type::view_type>;

private:
  static auto make_slice(typename mapped_type::const_view_type view,
                         const detail::slice_list &slices) {
    detail::do_make_slice(view, slices);
    return view;
  };

  struct make_item {
    const ConstView *view;
    template <class T> auto operator()(const T &item) const {
      return std::pair<Key, typename ConstView::mapped_type::const_view_type>(
          item.first, make_slice(item.second, view->slices()));
    }
  };

public:
  ConstView() = default;
  ConstView(holder_type &&items, const detail::slice_list &slices = {});

  /// Return the number of coordinates in the view.
  index size() const noexcept { return scipp::size(m_items); }
  /// Return true if there are 0 coordinates in the view.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  /// Returns whether a given key is present in the view.
  bool contains(const Key &k) const {
    return m_items.find(k) != m_items.cend();
  }

  /// Return a const view to the coordinate for given dimension.
  typename mapped_type::const_view_type operator[](const Key key) const {
    scipp::expect::contains(*this, key);
    return make_slice(m_items.at(key), m_slices);
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

  bool operator==(const ConstView &other) const;
  bool operator!=(const ConstView &other) const;

  const auto &items() const noexcept { return m_items; }
  const auto &slices() const noexcept { return m_slices; }

protected:
  holder_type m_items;
  detail::slice_list m_slices;
};

/// Common functionality for other view classes.
template <class Base, class Access> class MutableView : public Base {
private:
  static auto make_slice(typename Base::mapped_type::view_type view,
                         const detail::slice_list &slices) {
    detail::do_make_slice(view, slices);
    return view;
  };
  struct make_item {
    const MutableView<Base, Access> *view;
    template <class T> auto operator()(const T &item) const {
      return std::pair<typename Base::key_type,
                       typename Base::mapped_type::view_type>(
          item.first, make_slice(item.second, view->slices()));
    }
  };

  Access m_access;

public:
  MutableView() = default;
  MutableView(const Access &access, typename Base::holder_type &&items,
              const detail::slice_list &slices);
  /// Constructor for internal use (slicing and holding const view in mutable
  /// view)
  MutableView(const Access &access, Base &&base);

  /// Return a view to the coordinate for given dimension.
  typename Base::mapped_type::view_type
  operator[](const typename Base::key_type key) const {
    scipp::expect::contains(*this, key);
    return make_slice(Base::items().at(key), Base::slices());
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

  template <class VarOrView>
  void set(const typename Base::key_type key, VarOrView var) const {
    m_access.set(key, typename Base::mapped_type(std::move(var)));
  }

  void erase(const typename Base::key_type key) const {
    scipp::expect::contains(*this, key);
    m_access.erase(key);
  }
};

SCIPP_DATASET_EXPORT Variable irreducible_mask(const MasksConstView &masks,
                                               const Dim dim);

SCIPP_DATASET_EXPORT Variable
masks_merge_if_contained(const MasksConstView &masks, const Dimensions &dims);

} // namespace scipp::dataset
