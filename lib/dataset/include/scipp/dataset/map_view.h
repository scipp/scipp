// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/dict.h"
#include "scipp/core/sizes.h"
#include "scipp/core/slice.h"
#include "scipp/dataset/map_view_forward.h"
#include "scipp/units/dim.h"
#include "scipp/units/unit.h"
#include "scipp/variable/logical.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {
template <class Mapping>
Mapping slice_map(const Sizes &sizes, const Mapping &map, const Slice &params) {
  Mapping out;
  for (const auto &[key, value] : map) {
    if (value.dims().contains(params.dim())) {
      if (value.dims()[params.dim()] == sizes[params.dim()]) {
        out.insert_or_assign(key, value.slice(params));
      } else { // bin edge
        if (params.stride() != 1)
          throw except::SliceError(
              "Object has bin-edges along dimension " +
              to_string(params.dim()) + " so slicing with stride " +
              std::to_string(params.stride()) + " != 1 is not valid.");
        const auto end = params.end() == -1               ? params.begin() + 2
                         : params.begin() == params.end() ? params.end()
                                                          : params.end() + 1;
        out.insert_or_assign(
            key, value.slice(Slice{params.dim(), params.begin(), end}));
      }
    } else if (params == Slice{}) {
      out.insert_or_assign(key, value);
    } else {
      out.insert_or_assign(key, value.as_const());
    }
  }
  return out;
}

/// Common functionality for other const-view classes.
template <class Key, class Value> class AlignedDict {
public:
  using key_type = Key;
  using mapped_type = Value;
  using holder_type = core::Dict<key_type, mapped_type>;

  AlignedDict() = default;
  AlignedDict(const Sizes &sizes,
              std::initializer_list<std::pair<const Key, Value>> items,
              bool readonly = false);
  AlignedDict(Sizes sizes, holder_type items, bool readonly = false);
  AlignedDict(const AlignedDict &other);
  AlignedDict(AlignedDict &&other) noexcept;
  AlignedDict &operator=(const AlignedDict &other);
  AlignedDict &operator=(AlignedDict &&other) noexcept;

  /// Return the number of coordinates in the view.
  [[nodiscard]] index size() const noexcept { return scipp::size(m_items); }
  /// Return true if there are 0 coordinates in the view.
  [[nodiscard]] bool empty() const noexcept { return m_items.empty(); }
  void reserve(const index new_capacity) { m_items.reserve(new_capacity); }

  bool contains(const Key &k) const;
  scipp::index count(const Key &k) const;

  const mapped_type &operator[](const Key &key) const;
  const mapped_type &at(const Key &key) const;
  // Note that the non-const versions return by value, to avoid breakage of
  // invariants.
  mapped_type operator[](const Key &key);
  mapped_type at(const Key &key);
  Dim dim_of(const Key &key) const;

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
  auto keys_begin() const &noexcept { return m_items.keys_begin(); }
  auto keys_end() const && = delete;
  /// Return const iterator to the end of all keys.
  auto keys_end() const &noexcept { return m_items.keys_end(); }

  auto values_begin() const && = delete;
  /// Return const iterator to the beginning of all values.
  auto values_begin() const &noexcept { return m_items.values_begin(); }
  auto values_end() const && = delete;
  /// Return const iterator to the end of all values.
  auto values_end() const &noexcept { return m_items.values_end(); }

  bool operator==(const AlignedDict &other) const;
  bool operator!=(const AlignedDict &other) const;

  [[nodiscard]] const Sizes &sizes() const noexcept { return m_sizes; }
  [[nodiscard]] const auto &items() const noexcept { return m_items; }

  void setSizes(const Sizes &sizes);
  void rebuildSizes();
  void set(const key_type &key, mapped_type coord);
  void erase(const key_type &key);
  mapped_type extract(const key_type &key);
  mapped_type extract(const key_type &key, const mapped_type &default_value);

  AlignedDict slice(const Slice &params) const;
  std::tuple<AlignedDict, AlignedDict> slice_coords(const Slice &params) const;
  void validateSlice(const Slice &s, const AlignedDict &dict) const;
  [[maybe_unused]] AlignedDict &setSlice(const Slice &s,
                                         const AlignedDict &dict);

  void rename(Dim from, Dim to);

  void set_readonly() noexcept;
  [[nodiscard]] bool is_readonly() const noexcept;
  [[nodiscard]] AlignedDict as_const() const;
  [[nodiscard]] AlignedDict merge_from(const AlignedDict &other) const;

  bool item_applies_to(const Key &key, const Dimensions &dims) const;
  bool is_edges(const Key &key, std::optional<Dim> dim = std::nullopt) const;

protected:
  Sizes m_sizes;
  holder_type m_items;
  bool m_readonly{false};
};

/// Returns the union of all masks with irreducible dimension `dim`.
///
/// Irreducible means that a reduction operation must apply these masks since
/// they depend on the reduction dimension. Returns an invalid (empty) variable
/// if there is no irreducible mask.
template <class Masks>
[[nodiscard]] Variable irreducible_mask(const Masks &masks, const Dim dim) {
  Variable union_;
  for (const auto &mask : masks)
    if (mask.second.dims().contains(dim))
      union_ = union_.is_valid() ? union_ | mask.second : copy(mask.second);
  return union_;
}

template <class Key, class Value>
bool equals_nan(const AlignedDict<Key, Value> &a,
                const AlignedDict<Key, Value> &b);

template <class Key, class Value>
core::Dict<Key, Value> union_(const AlignedDict<Key, Value> &a,
                              const AlignedDict<Key, Value> &b,
                              std::string_view opname);

/// Return intersection of dicts, i.e., all items with matching names that
/// have matching content.
template <class Key, class Value>
core::Dict<Key, Value> intersection(const AlignedDict<Key, Value> &a,
                                    const AlignedDict<Key, Value> &b);

constexpr auto get_data = [](auto &&a) -> decltype(auto) { return a.data(); };
constexpr auto get_sizes = [](auto &&a) -> decltype(auto) { return a.sizes(); };
constexpr auto get_meta = [](auto &&a) -> decltype(auto) { return a.meta(); };
constexpr auto get_coords = [](auto &&a) -> decltype(auto) {
  return a.coords();
};
constexpr auto get_attrs = [](auto &&a) -> decltype(auto) { return a.attrs(); };
constexpr auto get_masks = [](auto &&a) -> decltype(auto) { return a.masks(); };

} // namespace scipp::dataset
