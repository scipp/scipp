// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <utility>

#include "scipp/dataset/aligned_dict.h"
#include "scipp/dataset/except.h"

namespace scipp::dataset {

namespace {
template <class T> void expect_writable(const T &dict) {
  if (dict.is_readonly())
    throw except::DataArrayError(
        "Read-only flag is set, cannot mutate metadata dict.");
}
} // namespace

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(
    const Sizes &sizes,
    std::initializer_list<std::pair<const Key, Value>> items,
    const bool readonly)
    : AlignedDict(sizes, holder_type(items), readonly) {}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(Sizes sizes, holder_type items,
                                     const bool readonly)
    : m_sizes(std::move(sizes)) {
  for (auto &&[key, value] : items)
    set(key, std::move(value));
  // `set` requires Dict to be writable, set readonly flag at the end.
  m_readonly = readonly; // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(const AlignedDict &other)
    : AlignedDict(other.m_sizes, other.m_items, false) {}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(AlignedDict &&other) noexcept
    : AlignedDict(std::move(other.m_sizes), std::move(other.m_items),
                  other.m_readonly) {}

template <class Key, class Value>
AlignedDict<Key, Value> &
AlignedDict<Key, Value>::operator=(const AlignedDict &other) = default;

template <class Key, class Value>
AlignedDict<Key, Value> &
AlignedDict<Key, Value>::operator=(AlignedDict &&other) noexcept = default;

template <class Key, class Value>
bool AlignedDict<Key, Value>::operator==(const AlignedDict &other) const {
  if (size() != other.size())
    return false;
  return std::all_of(this->begin(), this->end(), [&other](const auto &item) {
    const auto &[name, data] = item;
    return other.contains(name) && data == other[name];
  });
}

template <class Key, class Value>
bool equals_nan(const AlignedDict<Key, Value> &a,
                const AlignedDict<Key, Value> &b) {
  if (a.size() != b.size())
    return false;
  return std::all_of(a.begin(), a.end(), [&b](const auto &item) {
    const auto &[name, data] = item;
    return b.contains(name) && equals_nan(data, b[name]);
  });
}

template <class Key, class Value>
bool AlignedDict<Key, Value>::operator!=(const AlignedDict &other) const {
  return !operator==(other);
}

/// Returns whether a given key is present in the view.
template <class Key, class Value>
bool AlignedDict<Key, Value>::contains(const Key &k) const {
  return m_items.contains(k);
}

/// Returns 1 or 0, depending on whether key is present in the view or not.
template <class Key, class Value>
scipp::index AlignedDict<Key, Value>::count(const Key &k) const {
  return static_cast<scipp::index>(contains(k));
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value>
const Value &AlignedDict<Key, Value>::operator[](const Key &key) const {
  return at(key);
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value>
const Value &AlignedDict<Key, Value>::at(const Key &key) const {
  scipp::expect::contains(*this, key);
  return m_items[key];
}

/// The coordinate for given dimension.
template <class Key, class Value>
Value AlignedDict<Key, Value>::operator[](const Key &key) {
  return std::as_const(*this).at(key);
}

/// The coordinate for given dimension.
template <class Key, class Value>
Value AlignedDict<Key, Value>::at(const Key &key) {
  return std::as_const(*this).at(key);
}

/// Return the dimension for given coord.
/// @param key Key of the coordinate in a coord dict
///
/// Return the dimension of the coord for 1-D coords or Dim::Invalid for 0-D
/// coords. In the special case of multi-dimension coords the following applies,
/// in this order:
/// - For bin-edge coords return the dimension in which the coord dimension
///   exceeds the data dimensions.
/// - Else, for dimension coords (key matching a dimension), return the key.
/// - Else, return Dim::Invalid.
template <class Key, class Value>
Dim AlignedDict<Key, Value>::dim_of(const Key &key) const {
  const auto &var = at(key);
  if (var.dims().ndim() == 0)
    return Dim::Invalid;
  if (var.dims().ndim() == 1)
    return var.dims().inner();
  if constexpr (std::is_same_v<Key, Dim>) {
    for (const auto &dim : var.dims())
      if (core::is_edges(sizes(), var.dims(), dim))
        return dim;
    if (var.dims().contains(key))
      return key; // dimension coord
  }
  return Dim::Invalid;
}

template <class Key, class Value>
void AlignedDict<Key, Value>::setSizes(const Sizes &sizes) {
  scipp::expect::includes(sizes, m_sizes);
  m_sizes = sizes;
}

template <class Key, class Value> void AlignedDict<Key, Value>::rebuildSizes() {
  Sizes new_sizes = m_sizes;
  for (const auto &dim : m_sizes) {
    bool erase = true;
    for (const auto &item : *this) {
      if (item.second.dims().contains(dim)) {
        erase = false;
        break;
      }
    }
    if (erase)
      new_sizes.erase(dim);
  }
  m_sizes = std::move(new_sizes);
}

namespace {
template <class Key>
void expect_valid_coord_dims(const Key &key, const Dimensions &coord_dims,
                             const Sizes &da_sizes) {
  using core::to_string;
  if (!da_sizes.includes(coord_dims))
    throw except::DimensionError(
        "Cannot add coord '" + to_string(key) + "' of dims " +
        to_string(coord_dims) + " to DataArray with dims " +
        to_string(Dimensions{da_sizes.labels(), da_sizes.sizes()}));
}
} // namespace

template <class Key, class Value>
void AlignedDict<Key, Value>::set(const key_type &key, mapped_type coord) {
  if (contains(key) && at(key).is_same(coord))
    return;
  expect_writable(*this);
  // Is a good definition for things that are allowed: "would be possible to
  // concat along existing dim or extra dim"?
  auto dims = coord.dims();
  for (const auto &dim : coord.dims()) {
    if (!sizes().contains(dim) && dims[dim] == 2) { // bin edge along extra dim
      dims.erase(dim);
      break;
    } else if (dims[dim] == sizes()[dim] + 1) {
      dims.resize(dim, sizes()[dim]);
      break;
    }
  }
  expect_valid_coord_dims(key, dims, m_sizes);
  m_items.insert_or_assign(key, std::move(coord));
}

template <class Key, class Value>
void AlignedDict<Key, Value>::erase(const key_type &key) {
  expect_writable(*this);
  scipp::expect::contains(*this, key);
  m_items.erase(key);
}

template <class Key, class Value>
Value AlignedDict<Key, Value>::extract(const key_type &key) {
  expect_writable(*this);
  return m_items.extract(key);
}

template <class Key, class Value>
Value AlignedDict<Key, Value>::extract(const key_type &key,
                                       const mapped_type &default_value) {
  if (contains(key)) {
    return extract(key);
  }
  return default_value;
}

template <class Key, class Value>
AlignedDict<Key, Value>
AlignedDict<Key, Value>::slice(const Slice &params) const {
  const bool readonly = true;
  return {m_sizes.slice(params), slice_map(m_sizes, m_items, params), readonly};
}

namespace {
constexpr auto unaligned_by_dim_slice = [](const auto &coords, const auto &key,
                                           const auto &var,
                                           const Slice &params) {
  if (params == Slice{} || params.end() != -1)
    return false;
  const Dim dim = params.dim();
  return var.dims().contains(dim) && coords.dim_of(key) == dim;
};
}

template <class Key, class Value>
std::tuple<AlignedDict<Key, Value>, AlignedDict<Key, Value>>
AlignedDict<Key, Value>::slice_coords(const Slice &params) const {
  auto coords = slice(params);
  coords.m_readonly = false;
  AlignedDict<Key, Value> attrs(coords.sizes(), {});
  for (const auto &[key, var] : *this)
    if (unaligned_by_dim_slice(*this, key, var, params))
      attrs.set(key, coords.extract(key));
  coords.m_readonly = true;
  return {std::move(coords), std::move(attrs)};
}

template <class Key, class Value>
void AlignedDict<Key, Value>::validateSlice(const Slice &s,
                                            const AlignedDict &dict) const {
  using core::to_string;
  using units::to_string;
  for (const auto &[key, item] : dict) {
    const auto it = find(key);
    if (it == end()) {
      throw except::NotFoundError("Cannot insert new meta data '" +
                                  to_string(key) + "' via a slice.");
    } else if (const auto &var = it->second;
               (var.is_readonly() || !var.dims().contains(s.dim())) &&
               (var.dims().contains(s.dim()) ? var.slice(s) : var) != item) {
      throw except::DimensionError("Cannot update meta data '" +
                                   to_string(key) +
                                   "' via slice since it is implicitly "
                                   "broadcast along the slice dimension '" +
                                   to_string(s.dim()) + "'.");
    }
  }
}

template <class Key, class Value>
AlignedDict<Key, Value> &
AlignedDict<Key, Value>::setSlice(const Slice &s, const AlignedDict &dict) {
  validateSlice(s, dict);
  for (const auto &[key, item] : dict) {
    const auto it = find(key);
    if (it != end() && !it->second.is_readonly() &&
        it->second.dims().contains(s.dim()))
      it->second.setSlice(s, item);
  }
  return *this;
}

template <class Key, class Value>
void AlignedDict<Key, Value>::rename(const Dim from, const Dim to) {
  m_sizes.replace_key(from, to);
  for (auto it = m_items.values_begin(); it != m_items.values_end(); ++it)
    if ((*it).dims().contains(from))
      (*it).rename(from, to);
}

/// Mark the dict as readonly. Does not imply that items are readonly.
template <class Key, class Value>
void AlignedDict<Key, Value>::set_readonly() noexcept {
  m_readonly = true;
}

/// Return true if the dict is readonly. Does not imply that items are readonly.
template <class Key, class Value>
bool AlignedDict<Key, Value>::is_readonly() const noexcept {
  return m_readonly;
}

template <class Key, class Value>
AlignedDict<Key, Value> AlignedDict<Key, Value>::as_const() const {
  holder_type items;
  items.reserve(m_items.size());
  for (const auto &[key, val] : m_items)
    items.insert_or_assign(key, val.as_const());
  const bool readonly = true;
  return {sizes(), std::move(items), readonly};
}

template <class Key, class Value>
AlignedDict<Key, Value>
AlignedDict<Key, Value>::merge_from(const AlignedDict &other) const {
  using core::to_string;
  using units::to_string;
  auto out(*this);
  out.m_readonly = false;
  for (const auto &[key, value] : other) {
    if (out.contains(key))
      throw except::DataArrayError(
          "Coord '" + to_string(key) +
          "' shadows attr of the same name. Remove the attr if you are slicing "
          "an array or use the `coords` and `attrs` properties instead of "
          "`meta`.");
    out.set(key, value);
  }
  out.m_readonly = m_readonly;
  return out;
}

template <class Key, class Value>
bool AlignedDict<Key, Value>::item_applies_to(const Key &key,
                                              const Dimensions &dims) const {
  const auto &val = m_items[key];
  return std::all_of(val.dims().begin(), val.dims().end(),
                     [&dims](const Dim dim) { return dims.contains(dim); });
}

template <class Key, class Value>
bool AlignedDict<Key, Value>::is_edges(const Key &key,
                                       const std::optional<Dim> dim) const {
  const auto &val = this->at(key);
  return core::is_edges(m_sizes, val.dims(),
                        dim.has_value() ? *dim : val.dim());
}

template <class Key, class Value>
core::Dict<Key, Value> union_(const AlignedDict<Key, Value> &a,
                              const AlignedDict<Key, Value> &b,
                              std::string_view opname) {
  core::Dict<Key, Value> out;
  out.reserve(out.size() + b.size());
  for (const auto &[key, val] : a)
    out.insert_or_assign(key, val);
  for (const auto &[key, val] : b) {
    if (const auto it = a.find(key); it != a.end()) {
      expect::matching_coord(it->first, it->second, val, opname);
    } else
      out.insert_or_assign(key, val);
  }
  return out;
}

template <class Key, class Value>
core::Dict<Key, Value> intersection(const AlignedDict<Key, Value> &a,
                                    const AlignedDict<Key, Value> &b) {
  core::Dict<Key, Value> out;
  out.reserve(a.size());
  for (const auto &[key, item] : a)
    if (const auto it = b.find(key);
        it != b.end() && equals_nan(it->second, item))
      out.insert_or_assign(key, item);
  return out;
}

template class SCIPP_DATASET_EXPORT AlignedDict<Dim, Variable>;
template class SCIPP_DATASET_EXPORT AlignedDict<std::string, Variable>;
template SCIPP_DATASET_EXPORT bool equals_nan(const Coords &a, const Coords &b);
template SCIPP_DATASET_EXPORT bool equals_nan(const Masks &a, const Masks &b);
template SCIPP_DATASET_EXPORT typename Coords::holder_type
union_(const Coords &, const Coords &, std::string_view opname);
template SCIPP_DATASET_EXPORT typename Coords::holder_type
intersection(const Coords &, const Coords &);
// template SCIPP_DATASET_EXPORT typename Masks::holder_type intersection(const
// Masks &, const Masks &);
} // namespace scipp::dataset