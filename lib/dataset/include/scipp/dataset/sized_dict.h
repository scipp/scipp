// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/core/dict.h"
#include "scipp/core/sizes.h"
#include "scipp/core/slice.h"
#include "scipp/dataset/sized_dict_forward.h"
#include "scipp/units/dim.h"
#include "scipp/units/unit.h"
#include "scipp/variable/logical.h"
#include "scipp/variable/variable.h"

namespace scipp::dataset {
template <class Key, class Value>
auto empty_mapping_like([[maybe_unused]] const core::Dict<Key, Value> &mapping,
                        [[maybe_unused]] const Sizes &target_sizes) {
  return core::Dict<Key, Value>{};
}

template <class Key, class Value, class Impl>
auto empty_mapping_like(
    [[maybe_unused]] const SizedDict<Key, Value, Impl> &mapping,
    const Sizes &target_sizes) {
  return SizedDict<Key, Value, Impl>{target_sizes, {}};
}

template <class Key, class Value>
auto empty_mapping_like([[maybe_unused]] const AlignedDict<Key, Value> &mapping,
                        const Sizes &target_sizes) {
  return AlignedDict<Key, Value>{target_sizes, {}};
}

template <class Mapping>
Mapping slice_map(const Sizes &sizes, const Mapping &map, const Slice &params) {
  Mapping out = empty_mapping_like(map, sizes.slice(params));
  for (const auto &[key, value] : map) {
    if (value.dims().contains(params.dim())) {
      if (value.dims()[params.dim()] == sizes[params.dim()]) {
        out.set(key, value.slice(params));
      } else { // bin edge
        if (params.stride() != 1)
          throw except::SliceError(
              "Object has bin-edges along dimension " +
              to_string(params.dim()) + " so slicing with stride " +
              std::to_string(params.stride()) + " != 1 is not valid.");
        const auto end = params.end() == -1               ? params.begin() + 2
                         : params.begin() == params.end() ? params.end()
                                                          : params.end() + 1;
        out.set(key, value.slice(Slice{params.dim(), params.begin(), end}));
      }
    } else if (params == Slice{}) {
      out.set(key, value);
    } else {
      out.set(key, value.as_const());
    }
  }
  return out;
}

/// Dict with fixed dimensions.
///
/// Values must have dimensions and those dimensions must be a subset
/// of the sizes stored in SizedDict. This is used, e.g., to ensure
/// that coords are valid for a data array.
///
/// Template parameter Impl is used to specify return types of member functions
/// that return new instances of SizedDict. It can override the type to one
/// that can be constructed as Impl{SizedDict{...}}.
template <class Key, class Value, class Impl> class SizedDict {
public:
  using key_type = Key;
  using mapped_type = Value;
  using holder_type = core::Dict<key_type, mapped_type>;
  using impl =
      std::conditional_t<std::is_void_v<Impl>, SizedDict<Key, Value>, Impl>;

  SizedDict() = default;
  SizedDict(const Sizes &sizes,
            std::initializer_list<std::pair<const Key, Value>> items,
            bool readonly = false);
  SizedDict(Sizes sizes, holder_type items, bool readonly = false);
  SizedDict(const SizedDict &other);
  SizedDict(SizedDict &&other) noexcept;
  SizedDict &operator=(const SizedDict &other);
  SizedDict &operator=(SizedDict &&other) noexcept;

  /// Return the number of coordinates in the view.
  [[nodiscard]] index size() const noexcept { return scipp::size(m_items); }
  /// Return true if there are 0 coordinates in the view.
  [[nodiscard]] bool empty() const noexcept { return m_items.empty(); }
  /// Return the number of elements that space is currently allocated for.
  [[nodiscard]] index capacity() const noexcept { return m_items.capacity(); }
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

  bool operator==(const SizedDict &other) const;
  bool operator!=(const SizedDict &other) const;

  [[nodiscard]] const Sizes &sizes() const noexcept { return m_sizes; }
  [[nodiscard]] const auto &items() const noexcept { return m_items; }

  void setSizes(const Sizes &sizes);
  void rebuildSizes();
  void set(const key_type &key, mapped_type coord);
  void erase(const key_type &key);
  mapped_type extract(const key_type &key);
  mapped_type extract(const key_type &key, const mapped_type &default_value);
  // Like extract but without conversion in AlignedDict.
  mapped_type extract_raw(const key_type &key) { return extract(key); }

  impl slice(const Slice &params) const;
  void validateSlice(const Slice &s, const SizedDict &dict) const;
  [[maybe_unused]] void setSlice(const Slice &s, const SizedDict &dict);

  [[nodiscard]] impl rename_dims(const std::vector<std::pair<Dim, Dim>> &names,
                                 const bool fail_on_unknown = true) const;

  void set_readonly() noexcept;
  [[nodiscard]] bool is_readonly() const noexcept;
  [[nodiscard]] impl as_const() const;
  [[nodiscard]] SizedDict merge_from(const SizedDict &other) const;

  bool item_applies_to(const Key &key, const Dimensions &dims) const;
  bool is_edges(const Key &key, std::optional<Dim> dim = std::nullopt) const;

protected:
  Sizes m_sizes;
  holder_type m_items;
  bool m_readonly{false};
};

template <class Value> struct AlignedValue {
  Value value;
  bool aligned;

  AlignedValue(Value val) noexcept : value(std::move(val)), aligned(true) {}
  AlignedValue(Value val, const bool a) noexcept
      : value(std::move(val)), aligned(a) {}

  operator Value() const { return value; }

  bool operator==(const AlignedValue &other) const noexcept {
    return value == other.value && aligned == other.aligned;
  }

  bool operator!=(const AlignedValue &other) const noexcept {
    return !(*this == other);
  }

  [[nodiscard]] bool is_same(const AlignedValue &other) const noexcept {
    return value.is_same(other.value) && aligned == other.aligned;
  }

  [[nodiscard]] bool equals_nan(const AlignedValue &other) const noexcept {
    return equals_nan(value, other.value) && aligned == other.aligned;
  }

  [[nodiscard]] auto dim() const -> decltype(auto) { return value.dim(); }

  [[nodiscard]] const core::Dimensions &dims() const noexcept {
    return value.dims();
  }

  [[nodiscard]] AlignedValue slice(const Slice &params) const {
    return {value.slice(params), aligned};
  }

  void setSlice(const Slice &s, const Variable &data) {
    value.setSlice(s, data);
  }

  [[nodiscard]] AlignedValue
  rename_dims(const std::vector<std::pair<Dim, Dim>> &names,
              const bool fail_on_unknown = true) {
    return value.rename_dims(names, fail_on_unknown);
  }

  [[nodiscard]] AlignedValue as_const() const noexcept {
    return {value.as_const(), aligned};
  }

  [[nodiscard]] bool is_readonly() const noexcept {
    return value.is_readonly();
  }

  friend std::string to_string(const AlignedValue &x) {
    return to_string(x.value);
  }
};

/// Dict with fixed dimensions and alignment flag.
template <class Key, class Value>
class AlignedDict
    : public SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>> {
public:
  using key_type = Key;
  using mapped_type = Value;
  using aligned_mapped_type = AlignedValue<Value>;
  using holder_type = core::Dict<key_type, AlignedValue<Value>>;
  using raw_holder_type = core::Dict<key_type, Value>;

  AlignedDict() = default;
  AlignedDict(const Sizes &sizes,
              std::initializer_list<std::pair<const Key, Value>> items,
              bool readonly = false);
  AlignedDict(Sizes sizes, raw_holder_type items, bool readonly = false);
  AlignedDict(Sizes sizes, holder_type items, bool readonly = false);
  AlignedDict(const SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>>
                  &other);
  AlignedDict(SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>>
                  &&other) noexcept;
  AlignedDict(const AlignedDict &other);
  AlignedDict(AlignedDict &&other) noexcept;
  AlignedDict &operator=(const AlignedDict &other);
  AlignedDict &operator=(AlignedDict &&other) noexcept;

  const mapped_type &operator[](const Key &key) const;
  const mapped_type &at(const Key &key) const;
  const aligned_mapped_type &raw_at(const Key &key) const;
  // Note that the non-const versions return by value, to avoid breakage of
  // invariants.
  mapped_type operator[](const Key &key);
  mapped_type at(const Key &key);

  auto find(const Key &k) const noexcept {
    return this->m_items.find(k).transform(GetValue{});
  }
  auto find(const Key &k) noexcept {
    return this->m_items.find(k).transform(GetValue{});
  }

  /// Return const iterator to the beginning of all items.
  auto begin() const noexcept {
    return this->m_items.begin().transform(GetValue{});
  }
  auto begin() noexcept { return this->m_items.begin().transform(GetValue{}); }
  /// Return const iterator to the end of all items.
  auto end() const noexcept {
    return this->m_items.end().transform(GetValue{});
  }
  auto end() noexcept { return this->m_items.end().transform(GetValue{}); }

  auto items_begin() const && = delete;
  /// Return const iterator to the beginning of all items.
  auto items_begin() const &noexcept { return begin(); }
  auto items_end() const && = delete;
  /// Return const iterator to the end of all items.
  auto items_end() const &noexcept { return end(); }

  auto keys_begin() const && = delete;
  /// Return const iterator to the beginning of all keys.
  auto keys_begin() const &noexcept { return this->m_items.keys_begin(); }
  auto keys_end() const && = delete;
  /// Return const iterator to the end of all keys.
  auto keys_end() const &noexcept { return this->m_items.keys_end(); }

  auto values_begin() const && = delete;
  /// Return const iterator to the beginning of all values.
  auto values_begin() const &noexcept {
    return this->m_items.values_begin().transform(GetValue{});
  }
  auto values_end() const && = delete;
  /// Return const iterator to the end of all values.
  auto values_end() const &noexcept {
    return this->m_items.values_end().transform(GetValue{});
  }

  using SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>>::set;
  void set(const key_type &key, mapped_type coord, bool aligned = true);
  mapped_type extract(const key_type &key);
  mapped_type extract(const key_type &key, const mapped_type &default_value);

  std::tuple<AlignedDict, SizedDict<Key, Value>>
  slice_coords(const Slice &params) const;

  [[nodiscard]] AlignedDict merge_from(const AlignedDict &other) const;
  // TODO This is only used to merge attrs into coords in meta.
  //  Remove when we remove meta.
  [[nodiscard]] AlignedDict
  merge_from(const SizedDict<Key, Value> &other) const;

  [[nodiscard]] bool is_aligned(const Key &key) const noexcept;

private:
  struct GetValue {
    template <class T>
    constexpr auto operator()(T &&x) const noexcept -> decltype(auto) {
      if constexpr (std::is_same_v<std::decay_t<T>, aligned_mapped_type>)
        return std::forward<T>(x).value;
      else { // pair<Key, ValueHolder>
        auto &&[k, v] = x;
        return std::pair{std::forward<decltype(k)>(k),
                         std::forward<decltype(v)>(v).value};
      }
    }
  };
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
bool equals_nan(const SizedDict<Key, Value> &a, const SizedDict<Key, Value> &b);

template <class Key, class Value>
bool equals_nan(const AlignedDict<Key, Value> &a,
                const AlignedDict<Key, Value> &b);

template <class Key, class Value>
AlignedDict<Key, Value> union_(const AlignedDict<Key, Value> &a,
                               const AlignedDict<Key, Value> &b,
                               std::string_view opname);

/// Return intersection of dicts, i.e., all items with matching names that
/// have matching content.
template <class Key, class Value>
SizedDict<Key, Value> intersection(const SizedDict<Key, Value> &a,
                                   const SizedDict<Key, Value> &b);

constexpr auto get_data = [](auto &&a) -> decltype(auto) { return a.data(); };
constexpr auto get_sizes = [](auto &&a) -> decltype(auto) { return a.sizes(); };
constexpr auto get_meta = [](auto &&a) -> decltype(auto) { return a.meta(); };
constexpr auto get_coords = [](auto &&a) -> decltype(auto) {
  return a.coords();
};
constexpr auto get_attrs = [](auto &&a) -> decltype(auto) { return a.attrs(); };
constexpr auto get_masks = [](auto &&a) -> decltype(auto) { return a.masks(); };

} // namespace scipp::dataset
