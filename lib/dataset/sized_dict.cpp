// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <utility>

#include "scipp/dataset/except.h"
#include "scipp/dataset/sized_dict.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::dataset {

namespace {
template <class T> void expect_writable(const T &dict) {
  if (dict.is_readonly())
    throw except::DataArrayError(
        "Read-only flag is set, cannot mutate metadata dict.");
}

void merge_sizes_into(Sizes &target, const Dimensions &s) {
  using std::to_string;

  for (const auto &dim : s) {
    if (target.contains(dim)) {
      const auto a = target[dim];
      const auto b = s[dim];
      if (a == b + 1) // had bin-edges, replace by regular coord
        target.resize(dim, b);
      else if (a + 1 == b) { // had regular coord, got extra by bin-edges
        // keep current
      } else if (a != b)
        throw except::DimensionError(
            "Conflicting length in dimension " + to_string(dim) + ": " +
            to_string(target[dim]) + " vs " + to_string(s[dim]));
    } else {
      target.set(dim, s[dim]);
    }
  }
}

template <class Key, class Value>
auto make_from_items(typename SizedDict<Key, Value>::holder_type items,
                     const bool readonly) {
  Sizes sizes;
  for (auto &&[key, value] : items) {
    merge_sizes_into(sizes, value.dims());
  }
  return SizedDict<Key, Value>(std::move(sizes), std::move(items), readonly);
}
} // namespace

template <class Key, class Value>
SizedDict<Key, Value>::SizedDict(
    const Sizes &sizes,
    std::initializer_list<std::pair<const Key, Value>> items,
    const bool readonly)
    : SizedDict(sizes, holder_type(items), readonly) {}

template <class Key, class Value>
SizedDict<Key, Value>::SizedDict(
    AutoSizeTag tag, std::initializer_list<std::pair<const Key, Value>> items,
    const bool readonly)
    : SizedDict(tag, holder_type(items), readonly) {}

template <class Key, class Value>
SizedDict<Key, Value>::SizedDict(Sizes sizes, holder_type items,
                                 const bool readonly)
    : m_sizes(std::move(sizes)) {
  for (auto &&[key, value] : items)
    set(key, std::move(value));
  // `set` requires Dict to be writable, set readonly flag at the end.
  m_readonly = readonly; // NOLINT(cppcoreguidelines-prefer-member-initializer)
}

template <class Key, class Value>
SizedDict<Key, Value>::SizedDict(AutoSizeTag, holder_type items,
                                 const bool readonly)
    : SizedDict(make_from_items<Key, Value>(std::move(items), readonly)) {}

template <class Key, class Value>
SizedDict<Key, Value>::SizedDict(const SizedDict &other)
    : m_sizes(other.m_sizes), m_items(other.m_items), m_readonly(false) {}

template <class Key, class Value>
SizedDict<Key, Value>::SizedDict(SizedDict &&other) noexcept
    : m_sizes(std::move(other.m_sizes)), m_items(std::move(other.m_items)),
      m_readonly(other.m_readonly) {}

template <class Key, class Value>
SizedDict<Key, Value> &
SizedDict<Key, Value>::operator=(const SizedDict &other) = default;

template <class Key, class Value>
SizedDict<Key, Value> &
SizedDict<Key, Value>::operator=(SizedDict &&other) noexcept = default;

namespace {
template <class Item, class Key, class Value, class Compare>
bool item_in_other(const Item &item, const SizedDict<Key, Value> &other,
                   Compare &&compare_data) {
  const auto &[name, data] = item;
  if (!other.contains(name))
    return false;
  const auto &other_data = other[name];
  return compare_data(data, other_data) &&
         data.is_aligned() == other_data.is_aligned();
}
} // namespace

template <class Key, class Value>
bool SizedDict<Key, Value>::operator==(const SizedDict &other) const {
  if (size() != other.size())
    return false;
  return std::all_of(this->begin(), this->end(), [&other](const auto &item) {
    return item_in_other(item, other,
                         [](const auto &x, const auto &y) { return x == y; });
  });
}

template <class Key, class Value>
bool equals_nan(const SizedDict<Key, Value> &a,
                const SizedDict<Key, Value> &b) {
  if (a.size() != b.size())
    return false;
  return std::all_of(a.begin(), a.end(), [&b](const auto &item) {
    return item_in_other(
        item, b, [](const auto &x, const auto &y) { return equals_nan(x, y); });
  });
}

template <class Key, class Value>
bool SizedDict<Key, Value>::operator!=(const SizedDict &other) const {
  return !operator==(other);
}

/// Returns whether a given key is present in the view.
template <class Key, class Value>
bool SizedDict<Key, Value>::contains(const Key &k) const {
  return m_items.contains(k);
}

/// Returns 1 or 0, depending on whether key is present in the view or not.
template <class Key, class Value>
scipp::index SizedDict<Key, Value>::count(const Key &k) const {
  return static_cast<scipp::index>(contains(k));
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value>
const Value &SizedDict<Key, Value>::operator[](const Key &key) const {
  return at(key);
}

/// Const reference to the coordinate for given dimension.
template <class Key, class Value>
const Value &SizedDict<Key, Value>::at(const Key &key) const {
  scipp::expect::contains(*this, key);
  return m_items.at(key);
}

/// The coordinate for given dimension.
template <class Key, class Value>
Value SizedDict<Key, Value>::operator[](const Key &key) {
  return std::as_const(*this).at(key);
}

/// The coordinate for given dimension.
template <class Key, class Value>
Value SizedDict<Key, Value>::at(const Key &key) {
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
Dim SizedDict<Key, Value>::dim_of(const Key &key) const {
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
void SizedDict<Key, Value>::setSizes(const Sizes &sizes) {
  scipp::expect::includes(sizes, m_sizes);
  m_sizes = sizes;
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
void SizedDict<Key, Value>::set(const key_type &key, mapped_type coord) {
  if (contains(key) && at(key).is_same(coord))
    return;
  expect_writable(*this);
  using core::to_string;
  if (is_bins(coord))
    throw except::VariableError(
        std::string("Cannot set binned variable as coord or mask.\n") +
        "When working with binned data, binned coords or masks are typically "
        "set via the `bins` property.\nInstead of\n"
        "    da.coords[" +
        to_string(key) + "] = binned_var`\n" +
        "use\n"
        "    da.bins.coords[" +
        to_string(key) + "] = binned_var`");
  auto dims = coord.dims();
  // Is a good definition for things that are allowed: "would be possible to
  // concat along existing dim or extra dim"?
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
void SizedDict<Key, Value>::erase(const key_type &key) {
  static_cast<void>(extract(key));
}

template <class Key, class Value>
Value SizedDict<Key, Value>::extract(const key_type &key) {
  expect_writable(*this);
  return m_items.extract(key);
}

template <class Key, class Value>
Value SizedDict<Key, Value>::extract(const key_type &key,
                                     const mapped_type &default_value) {
  if (contains(key)) {
    return extract(key);
  }
  return default_value;
}

template <class Key, class Value>
SizedDict<Key, Value> SizedDict<Key, Value>::slice(const Slice &params) const {
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
} // namespace

template <class Key, class Value>
SizedDict<Key, Value>
SizedDict<Key, Value>::slice_coords(const Slice &params) const {
  auto coords = slice(params);
  coords.m_readonly = false;
  for (const auto &[key, var] : *this)
    if (unaligned_by_dim_slice(*this, key, var, params))
      coords.set_aligned(key, false);
  coords.m_readonly = true;
  return coords;
}

template <class Key, class Value>
void SizedDict<Key, Value>::validateSlice(const Slice &s,
                                          const SizedDict &dict) const {
  using core::to_string;
  using sc_units::to_string;
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
SizedDict<Key, Value> &SizedDict<Key, Value>::setSlice(const Slice &s,
                                                       const SizedDict &dict) {
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
SizedDict<Key, Value> SizedDict<Key, Value>::rename_dims(
    const std::vector<std::pair<Dim, Dim>> &names,
    const bool fail_on_unknown) const {
  auto out(*this);
  out.m_sizes = out.m_sizes.rename_dims(names, fail_on_unknown);
  for (auto &&item : out.m_items) {
    // DataArray coords support the special case of length-2 items with a
    // dim that is not contained in the data array dims. This occurs, e.g., when
    // slicing along a dim that has a bin edge coord. We must prevent renaming
    // to such dims. This is the reason for calling with `names` that may
    // contain unknown dims (and the `fail_on_unknown` arg). Otherwise the
    // caller would need to perform this check.
    for (const auto &rename : names)
      if (!m_sizes.contains(rename.second) &&
          item.second.dims().contains(rename.second))
        throw except::DimensionError("Duplicate dimension " +
                                     sc_units::to_string(rename.second) + ".");
    item.second = item.second.rename_dims(names, false);
  }
  return out;
}

/// Mark the dict as readonly. Does not imply that items are readonly.
template <class Key, class Value>
void SizedDict<Key, Value>::set_readonly() noexcept {
  m_readonly = true;
}

/// Return true if the dict is readonly. Does not imply that items are readonly.
template <class Key, class Value>
bool SizedDict<Key, Value>::is_readonly() const noexcept {
  return m_readonly;
}

template <class Key, class Value>
SizedDict<Key, Value> SizedDict<Key, Value>::as_const() const {
  holder_type items;
  items.reserve(m_items.size());
  for (const auto &[key, val] : m_items)
    items.insert_or_assign(key, val.as_const());
  const bool readonly = true;
  return {sizes(), std::move(items), readonly};
}

template <class Key, class Value>
SizedDict<Key, Value>
SizedDict<Key, Value>::merge_from(const SizedDict &other) const {
  using core::to_string;
  using sc_units::to_string;

  auto out(*this);
  out.m_readonly = false;
  for (const auto &[key, value] : other) {
    out.set(key, value);
  }
  out.m_readonly = m_readonly;
  return out;
}

template <class Key, class Value>
bool SizedDict<Key, Value>::item_applies_to(const Key &key,
                                            const Dimensions &dims) const {
  const auto &val = m_items.at(key);
  return std::all_of(val.dims().begin(), val.dims().end(),
                     [&dims](const Dim dim) { return dims.contains(dim); });
}

template <class Key, class Value>
bool SizedDict<Key, Value>::is_edges(const Key &key,
                                     const std::optional<Dim> dim) const {
  const auto &val = this->at(key);
  if (!dim.has_value() && val.dims().ndim() > 1) {
    throw except::DimensionError(
        "Expected 1d coordinate, or a dimension name in the second argument. "
        "But coord is multi-dimensional, and no dimension name was specified. "
        "Use the second argument to specify what dimension to check for "
        "bin-edges.");
  }
  return core::is_edges(m_sizes, val.dims(),
                        dim.has_value() ? *dim : val.dim());
}

template <class Key, class Value>
void SizedDict<Key, Value>::set_aligned(const Key &key, const bool aligned) {
  expect_writable(*this);
  m_items.at(key).set_aligned(aligned);
}

template <class Key, class Value>
core::Dict<Key, Value> union_(const SizedDict<Key, Value> &a,
                              const SizedDict<Key, Value> &b,
                              std::string_view opname) {
  core::Dict<Key, Value> out;
  out.reserve(a.size() + b.size());
  for (const auto &[key, val_a] : a)
    if (val_a.is_aligned())
      out.insert_or_assign(key, val_a);

  for (const auto &[key, val_b] : b) {
    if (const auto it = a.find(key); it != a.end()) {
      auto &&val_a = it->second;
      if (val_a.is_aligned() && val_b.is_aligned())
        expect::matching_coord(key, val_a, val_b, opname);
      else if (val_b.is_aligned())
        // aligned b takes precedence over unaligned a
        out.insert_or_assign(key, val_b);
      else if (!val_a.is_aligned()) {
        // neither is aligned
        if (equals_nan(val_a, val_b))
          out.insert_or_assign(key, val_b);
        // else: mismatching unaligned coords => do not include in out
      }
      // else: aligned a takes precedence over unaligned b
    } else {
      if (val_b.is_aligned())
        out.insert_or_assign(key, val_b);
    }
  }

  return out;
}

template <class Key, class Value>
core::Dict<Key, Value> intersection(const SizedDict<Key, Value> &a,
                                    const SizedDict<Key, Value> &b) {
  core::Dict<Key, Value> out;
  for (const auto &[key, item] : a)
    if (const auto it = b.find(key);
        it != b.end() && equals_nan(it->second, item))
      out.insert_or_assign(key, item);
  return out;
}

template class SCIPP_DATASET_EXPORT SizedDict<Dim, Variable>;
template class SCIPP_DATASET_EXPORT SizedDict<std::string, Variable>;
template SCIPP_DATASET_EXPORT bool equals_nan(const Coords &a, const Coords &b);
template SCIPP_DATASET_EXPORT bool equals_nan(const Masks &a, const Masks &b);
template SCIPP_DATASET_EXPORT typename Coords::holder_type
union_(const Coords &, const Coords &, std::string_view opname);
template SCIPP_DATASET_EXPORT typename Coords::holder_type
intersection(const Coords &, const Coords &);
} // namespace scipp::dataset
