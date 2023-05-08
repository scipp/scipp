// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <utility>

#include "scipp/dataset/except.h"
#include "scipp/dataset/sized_dict.h"

namespace scipp::dataset {

namespace {
template <class T> void expect_writable(const T &dict) {
  if (dict.is_readonly())
    throw except::DataArrayError(
        "Read-only flag is set, cannot mutate metadata dict.");
}
} // namespace

template <class Key, class Value, class Impl>
SizedDict<Key, Value, Impl>::SizedDict(
    const Sizes &sizes,
    std::initializer_list<std::pair<const Key, Value>> items,
    const bool readonly)
    : SizedDict(sizes, holder_type(items), readonly) {}

template <class Key, class Value, class Impl>
SizedDict<Key, Value, Impl>::SizedDict(Sizes sizes, holder_type items,
                                       const bool readonly)
    : m_sizes(std::move(sizes)) {
  for (auto &&[key, value] : items)
    set(key, std::move(value));
  // `set` requires Dict to be writable, set readonly flag at the end.
  m_readonly = readonly; // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

template <class Key, class Value, class Impl>
SizedDict<Key, Value, Impl>::SizedDict(const SizedDict &other)
    : SizedDict(other.m_sizes, other.m_items, false) {}

template <class Key, class Value, class Impl>
SizedDict<Key, Value, Impl>::SizedDict(SizedDict &&other) noexcept
    : SizedDict(std::move(other.m_sizes), std::move(other.m_items),
                other.m_readonly) {}

template <class Key, class Value, class Impl>
SizedDict<Key, Value, Impl> &
SizedDict<Key, Value, Impl>::operator=(const SizedDict &other) = default;

template <class Key, class Value, class Impl>
SizedDict<Key, Value, Impl> &
SizedDict<Key, Value, Impl>::operator=(SizedDict &&other) noexcept = default;

template <class Key, class Value, class Impl>
bool SizedDict<Key, Value, Impl>::operator==(const SizedDict &other) const {
  if (size() != other.size())
    return false;
  return std::all_of(this->begin(), this->end(), [&other](const auto &item) {
    const auto &[name, data] = item;
    return other.contains(name) && data == other[name];
  });
}

template <class Key, class Value, class Impl>
bool SizedDict<Key, Value, Impl>::operator!=(const SizedDict &other) const {
  return !operator==(other);
}

/// Returns whether a given key is present in the view.
template <class Key, class Value, class Impl>
bool SizedDict<Key, Value, Impl>::contains(const Key &k) const {
  return m_items.contains(k);
}

/// Returns 1 or 0, depending on whether key is present in the view or not.
template <class Key, class Value, class Impl>
scipp::index SizedDict<Key, Value, Impl>::count(const Key &k) const {
  return static_cast<scipp::index>(contains(k));
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value, class Impl>
const Value &SizedDict<Key, Value, Impl>::operator[](const Key &key) const {
  return at(key);
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value, class Impl>
const Value &SizedDict<Key, Value, Impl>::at(const Key &key) const {
  if (const auto it = m_items.find(key); it == m_items.end()) {
    using core::to_string;
    throw except::NotFoundError("Expected " + to_string(*this) +
                                " to contain " + to_string(key) + ".");
  } else {
    return it->second;
  }
}

/// The coordinate for given dimension.
template <class Key, class Value, class Impl>
Value SizedDict<Key, Value, Impl>::operator[](const Key &key) {
  return std::as_const(*this).at(key);
}

/// The coordinate for given dimension.
template <class Key, class Value, class Impl>
Value SizedDict<Key, Value, Impl>::at(const Key &key) {
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
template <class Key, class Value, class Impl>
Dim SizedDict<Key, Value, Impl>::dim_of(const Key &key) const {
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

template <class Key, class Value, class Impl>
void SizedDict<Key, Value, Impl>::setSizes(const Sizes &sizes) {
  scipp::expect::includes(sizes, m_sizes);
  m_sizes = sizes;
}

template <class Key, class Value, class Impl>
void SizedDict<Key, Value, Impl>::rebuildSizes() {
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

template <class Key, class Value, class Impl>
void SizedDict<Key, Value, Impl>::set(const key_type &key, mapped_type coord) {
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

template <class Key, class Value, class Impl>
void SizedDict<Key, Value, Impl>::erase(const key_type &key) {
  static_cast<void>(extract(key));
}

template <class Key, class Value, class Impl>
Value SizedDict<Key, Value, Impl>::extract(const key_type &key) {
  expect_writable(*this);
  return m_items.extract(key);
}

template <class Key, class Value, class Impl>
Value SizedDict<Key, Value, Impl>::extract(const key_type &key,
                                           const mapped_type &default_value) {
  if (contains(key)) {
    return extract(key);
  }
  return default_value;
}

template <class Key, class Value, class Impl>
typename SizedDict<Key, Value, Impl>::impl
SizedDict<Key, Value, Impl>::slice(const Slice &params) const {
  auto sliced = slice_map(sizes(), *this, params);
  sliced.setSizes(this->m_sizes.slice(params));
  sliced.set_readonly();
  return typename SizedDict<Key, Value, Impl>::impl{std::move(sliced)};
}

template <class Key, class Value, class Impl>
void SizedDict<Key, Value, Impl>::validateSlice(const Slice &s,
                                                const SizedDict &dict) const {
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

template <class Key, class Value, class Impl>
void SizedDict<Key, Value, Impl>::setSlice(const Slice &s,
                                           const SizedDict &dict) {
  validateSlice(s, dict);
  for (const auto &[key, item] : dict) {
    const auto it = find(key);
    if (it != end() && !it->second.is_readonly() &&
        it->second.dims().contains(s.dim()))
      it->second.setSlice(s, item);
  }
}

template <class Key, class Value, class Impl>
typename SizedDict<Key, Value, Impl>::impl
SizedDict<Key, Value, Impl>::rename_dims(
    const std::vector<std::pair<Dim, Dim>> &names,
    const bool fail_on_unknown) const {
  impl out(*this);
  out.m_sizes = out.m_sizes.rename_dims(names, fail_on_unknown);
  for (auto &&item : out.m_items) {
    // DataArray coords/attrs support the special case of length-2 items with a
    // dim that is not contained in the data array dims. This occurs, e.g., when
    // slicing along a dim that has a bin edge coord. We must prevent renaming
    // to such dims. This is the reason for calling with `names` that may
    // contain unknown dims (and the `fail_on_unknown` arg). Otherwise the
    // caller would need to perform this check.
    for (const auto &rename : names)
      if (!m_sizes.contains(rename.second) &&
          item.second.dims().contains(rename.second))
        throw except::DimensionError("Duplicate dimension " +
                                     units::to_string(rename.second) + ".");
    item.second = item.second.rename_dims(names, false);
  }
  return out;
}

/// Mark the dict as readonly. Does not imply that items are readonly.
template <class Key, class Value, class Impl>
void SizedDict<Key, Value, Impl>::set_readonly() noexcept {
  m_readonly = true;
}

/// Return true if the dict is readonly. Does not imply that items are readonly.
template <class Key, class Value, class Impl>
bool SizedDict<Key, Value, Impl>::is_readonly() const noexcept {
  return m_readonly;
}

template <class Key, class Value, class Impl>
typename SizedDict<Key, Value, Impl>::impl
SizedDict<Key, Value, Impl>::as_const() const {
  holder_type items;
  items.reserve(m_items.size());
  for (const auto &[key, val] : m_items)
    items.insert_or_assign(key, val.as_const());
  const bool readonly = true;
  return {sizes(), std::move(items), readonly};
}

template <class Key, class Value, class Impl>
SizedDict<Key, Value, Impl>
SizedDict<Key, Value, Impl>::merge_from(const SizedDict &other) const {
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

template <class Key, class Value, class Impl>
bool SizedDict<Key, Value, Impl>::item_applies_to(
    const Key &key, const Dimensions &dims) const {
  const auto &val = m_items.at(key);
  return std::all_of(val.dims().begin(), val.dims().end(),
                     [&dims](const Dim dim) { return dims.contains(dim); });
}

template <class Key, class Value, class Impl>
bool SizedDict<Key, Value, Impl>::is_edges(const Key &key,
                                           const std::optional<Dim> dim) const {
  const auto &val = this->at(key);
  return core::is_edges(m_sizes, val.dims(),
                        dim.has_value() ? *dim : val.dim());
}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(
    const Sizes &sizes,
    std::initializer_list<std::pair<const Key, Value>> items,
    const bool readonly)
    : AlignedDict(sizes, raw_holder_type(items), readonly) {}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(Sizes sizes, raw_holder_type items,
                                     const bool readonly) {
  this->m_sizes = std::move(sizes);
  for (auto &&[key, value] : items)
    set(key, std::move(value));
  // `set` requires Dict to be writable, set readonly flag at the end.
  this->m_readonly =
      readonly; // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(Sizes sizes, holder_type items,
                                     const bool readonly) {
  this->m_sizes = std::move(sizes);
  for (auto &&[key, value] : items)
    set(key, std::move(value.value), value.aligned);
  // `set` requires Dict to be writable, set readonly flag at the end.
  this->m_readonly =
      readonly; // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(
    const SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>> &other)
    : SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>>{other} {}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(
    SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>>
        &&other) noexcept
    : SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>>{
          std::move(other)} {}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(const AlignedDict &other)
    : AlignedDict(other.m_sizes, other.m_items, false) {}

template <class Key, class Value>
AlignedDict<Key, Value>::AlignedDict(AlignedDict &&other) noexcept = default;

template <class Key, class Value>
AlignedDict<Key, Value> &
AlignedDict<Key, Value>::operator=(const AlignedDict &other) = default;

template <class Key, class Value>
AlignedDict<Key, Value> &
AlignedDict<Key, Value>::operator=(AlignedDict &&other) noexcept = default;

/// Const reference to the coordinate for given dimension.
template <class Key, class Value>
const Value &AlignedDict<Key, Value>::operator[](const Key &key) const {
  return at(key);
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value>
const Value &AlignedDict<Key, Value>::at(const Key &key) const {
  return raw_at(key).value;
}

/// The coordinate for given dimension.
template <class Key, class Value>
Value AlignedDict<Key, Value>::operator[](const Key &key) {
  return std::as_const(*this).at(key);
}

template <class Key, class Value>
const AlignedValue<Value> &
AlignedDict<Key, Value>::raw_at(const Key &key) const {
  return SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>>::at(key);
}

/// The coordinate for given dimension.
template <class Key, class Value>
Value AlignedDict<Key, Value>::at(const Key &key) {
  return std::as_const(*this).at(key);
}

template <class Key, class Value>
void AlignedDict<Key, Value>::set(const key_type &key, mapped_type coord,
                                  const bool aligned) {
  set(key, AlignedValue<Value>{std::move(coord), aligned});
}

template <class Key, class Value>
Value AlignedDict<Key, Value>::extract(const key_type &key) {
  return SizedDict<Key, AlignedValue<Value>, AlignedDict<Key, Value>>::extract(
             key)
      .value;
}

template <class Key, class Value>
Value AlignedDict<Key, Value>::extract(const key_type &key,
                                       const mapped_type &default_value) {
  if (this->contains(key)) {
    return extract(key);
  }
  return default_value;
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

// TODO change to set alignment flag instead of splitting into coords+attrs
template <class Key, class Value>
std::tuple<AlignedDict<Key, Value>, AlignedDict<Key, Value>>
AlignedDict<Key, Value>::slice_coords(const Slice &params) const {
  auto coords = this->slice(params);
  coords.m_readonly = false;
  AlignedDict<Key, Value> attrs(coords.sizes(), {});
  for (const auto &[key, var] : *this)
    if (unaligned_by_dim_slice(*this, key, var, params))
      attrs.set(key, coords.extract(key));
  coords.m_readonly = true;
  return {std::move(coords), std::move(attrs)};
}

template <class Key, class Value>
AlignedDict<Key, Value>
AlignedDict<Key, Value>::merge_from(const AlignedDict &other) const {
  using core::to_string;
  using units::to_string;
  auto out(*this);
  out.m_readonly = false;
  for (const auto &[key, aligned_value] : other.m_items) {
    if (out.contains(key))
      throw except::DataArrayError(
          "Coord '" + to_string(key) +
          "' shadows attr of the same name. Remove the attr if you are slicing "
          "an array or use the `coords` and `attrs` properties instead of "
          "`meta`.");
    out.set(key, aligned_value.value, aligned_value.aligned);
  }
  out.m_readonly = this->m_readonly;
  return out;
}

template <class Key, class Value>
bool AlignedDict<Key, Value>::is_aligned(const Key &key) const noexcept {
  return this->m_items[key].aligned;
}

template <class Mapping>
bool equals_nan_impl(const Mapping &a, const Mapping &b) {
  if (a.size() != b.size())
    return false;
  return std::all_of(a.begin(), a.end(), [&b](const auto &item) {
    const auto &[name, data] = item;
    return b.contains(name) && equals_nan(data, b[name]);
  });
}

template <class Key, class Value>
bool equals_nan(const SizedDict<Key, Value> &a,
                const SizedDict<Key, Value> &b) {
  return equals_nan_impl(a, b);
}

template <class Key, class Value>
bool equals_nan(const AlignedDict<Key, Value> &a,
                const AlignedDict<Key, Value> &b) {
  return equals_nan_impl(a, b);
}

template <class Key, class Value>
AlignedDict<Key, Value> union_(const AlignedDict<Key, Value> &a,
                               const AlignedDict<Key, Value> &b,
                               std::string_view opname) {
  auto out = a;
  out.setSizes(merge(a.sizes(), b.sizes()));
  out.reserve(out.size() + b.size());
  for (const auto &[key, val] : b) {
    if (const auto it = a.find(key); it != a.end()) {
      expect::matching_coord(it->first, it->second, val, opname);
    } else
      out.set(key, val, b.is_aligned(key));
  }
  return out;
}

template <class Key, class Value>
AlignedDict<Key, Value> intersection(const AlignedDict<Key, Value> &a,
                                     const AlignedDict<Key, Value> &b) {
  AlignedDict<Key, Value> out(merge(a.sizes(), b.sizes()), {});
  for (const auto &[key, item] : a)
    if (const auto it = b.find(key);
        it != b.end() && equals_nan(it->second, item))
      out.set(key, item);
  return out;
}

template class SCIPP_DATASET_EXPORT SizedDict<std::string, Variable>;
template class SCIPP_DATASET_EXPORT
    SizedDict<Dim, AlignedValue<Variable>, AlignedDict<Dim, Variable>>;
template class SCIPP_DATASET_EXPORT AlignedDict<Dim, Variable>;
template SCIPP_DATASET_EXPORT bool equals_nan(const Coords &a, const Coords &b);
template SCIPP_DATASET_EXPORT bool equals_nan(const Masks &a, const Masks &b);
template SCIPP_DATASET_EXPORT Coords union_(const Coords &, const Coords &,
                                            std::string_view opname);
template SCIPP_DATASET_EXPORT Attrs intersection(const Attrs &, const Attrs &);
} // namespace scipp::dataset
