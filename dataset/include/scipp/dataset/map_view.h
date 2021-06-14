// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <boost/container/small_vector.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include "scipp/core/sizes.h"
#include "scipp/core/slice.h"
#include "scipp/dataset/map_view_forward.h"
#include "scipp/units/dim.h"
#include "scipp/units/unit.h"
#include "scipp/variable/logical.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {

namespace detail {
using slice_list =
    boost::container::small_vector<std::pair<Slice, scipp::index>, 2>;

static constexpr auto make_key_value = [](auto &&view) {
  using In = decltype(view);
  using View =
      std::conditional_t<std::is_rvalue_reference_v<In>, std::decay_t<In>, In>;
  return std::pair<std::string, View>(view.name(),
                                      std::forward<decltype(view)>(view));
};

static constexpr auto make_key = [](auto &&view) -> decltype(auto) {
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
/// dimension. Returns Dim::Invalid for 0-D var.
template <class T, class Key> Dim dim_of_coord(const T &var, const Key &key) {
  if (var.dims().ndim() == 0)
    return Dim::Invalid;
  if constexpr (std::is_same_v<Key, Dim>) {
    const bool is_dimension_coord = var.dims().contains(key);
    return is_dimension_coord ? key : var.dims().inner();
  } else
    return var.dims().inner();
}

template <class T>
auto slice_map(const Sizes &sizes, const T &map, const Slice &params) {
  T out;
  for (const auto &[key, value] : map) {
    if (value.dims().contains(params.dim())) {
      if (value.dims()[params.dim()] == sizes[params.dim()]) {
        out[key] = value.slice(params);
      } else { // bin edge
        const auto end = params.end() == -1 ? params.begin() + 2
                                            : params.begin() == params.end()
                                                  ? params.end()
                                                  : params.end() + 1;
        out[key] = value.slice(Slice{params.dim(), params.begin(), end});
      }
    } else if (params == Slice{}) {
      out[key] = value;
    } else {
      out[key] = value.as_const();
    }
  }
  return out;
}

/// Common functionality for other const-view classes.
template <class Key, class Value> class Dict {
public:
  using key_type = Key;
  using mapped_type = Value;
  using holder_type = std::unordered_map<key_type, mapped_type>;

  Dict() = default;
  Dict(const Sizes &sizes,
       std::initializer_list<std::pair<const Key, Value>> items,
       const bool readonly = false);
  Dict(const Sizes &sizes, holder_type items, const bool readonly = false);
  Dict(const Dict &other);
  Dict(Dict &&other);
  Dict &operator=(const Dict &other);
  Dict &operator=(Dict &&other);

  /// Return the number of coordinates in the view.
  index size() const noexcept { return scipp::size(m_items); }
  /// Return true if there are 0 coordinates in the view.
  [[nodiscard]] bool empty() const noexcept { return size() == 0; }

  bool contains(const Key &k) const;
  scipp::index count(const Key &k) const;

  const mapped_type &operator[](const Key &key) const;
  const mapped_type &at(const Key &key) const;
  // Note that the non-const versions return by value, to avoid breakage of
  // invariants.
  mapped_type operator[](const Key &key);
  mapped_type at(const Key &key);

  auto find(const Key &k) const noexcept { return m_items.find(k); }
  auto find(const Key &k) noexcept { return m_items.find(k); }

  /// Return const iterator to the beginning of all items.
  auto begin() const noexcept { return m_items.begin(); }
  auto begin() noexcept { return m_items.begin(); }
  /// Return const iterator to the end of all items.
  auto end() const noexcept { return m_items.end(); }
  auto end() noexcept { return m_items.end(); }

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

  bool operator==(const Dict &other) const;
  bool operator!=(const Dict &other) const;

  const Sizes &sizes() const noexcept { return m_sizes; }
  const auto &items() const noexcept { return m_items; }

  void setSizes(const Sizes &sizes);
  void rebuildSizes();
  void set(const key_type &key, mapped_type coord);
  void erase(const key_type &key);
  mapped_type extract(const key_type &key);

  Dict slice(const Slice &params) const;
  std::tuple<Dict, Dict> slice_coords(const Slice &params) const;
  void validateSlice(const Slice s, const Dict &dict) const;
  [[maybe_unused]] Dict &setSlice(const Slice s, const Dict &dict);

  void rename(const Dim from, const Dim to);

  bool is_readonly() const noexcept;
  [[nodiscard]] Dict as_const() const;
  [[nodiscard]] Dict merge_from(const Dict &other) const;

  bool item_applies_to(const Key &key, const Dimensions &dims) const;

protected:
  Sizes m_sizes;
  holder_type m_items;
  bool m_readonly{false};
};

/// Returns the union of all masks with irreducible dimension `dim`.
///
/// Irreducible means that a reduction operation must apply these masks since
/// depend on the reduction dimension. Returns an invalid (empty) variable if
/// there is no irreducible mask.
template <class Masks>
[[nodiscard]] Variable irreducible_mask(const Masks &masks, const Dim dim) {
  Variable union_;
  for (const auto &mask : masks)
    if (mask.second.dims().contains(dim))
      union_ = union_.is_valid() ? union_ | mask.second : copy(mask.second);
  return union_;
}

SCIPP_DATASET_EXPORT Variable masks_merge_if_contained(const Masks &masks,
                                                       const Dimensions &dims);

[[nodiscard]] SCIPP_DATASET_EXPORT Coords copy(const Coords &coords);
[[nodiscard]] SCIPP_DATASET_EXPORT Masks copy(const Masks &masks);

} // namespace scipp::dataset
