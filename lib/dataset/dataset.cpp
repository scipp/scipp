// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset.h"
#include "scipp/core/except.h"
#include "scipp/dataset/dataset_util.h"
#include "scipp/dataset/except.h"

namespace scipp::dataset {

namespace {
template <class T> void expect_writable(const T &dict) {
  if (dict.is_readonly())
    throw except::DatasetError(
        "Read-only flag is set, cannot insert new or erase existing items.");
}
} // namespace

Dataset::Dataset(const Dataset &other)
    : m_coords(other.m_coords), m_data(other.m_data), m_readonly(false) {}

Dataset::Dataset(const DataArray &data) { setData(data.name(), data); }

Dataset &Dataset::operator=(const Dataset &other) {
  return *this = Dataset(other);
}

Dataset &Dataset::operator=(Dataset &&other) {
  if (this == &other) {
    return *this;
  }
  check_nested_in_assign(*this, other);
  m_coords = std::move(other.m_coords);
  m_data = std::move(other.m_data);
  m_readonly = other.m_readonly;
  return *this;
}

/// Removes all data items from the Dataset.
///
/// Coordinates are not modified.
void Dataset::clear() {
  expect_writable(*this);
  m_data.clear();
  rebuildDims();
}

void Dataset::setCoords(Coords other) {
  scipp::expect::includes(other.sizes(), m_coords.sizes());
  m_coords = std::move(other);
}
/// Return a const view to all coordinates of the dataset.
const Coords &Dataset::coords() const noexcept { return m_coords; }

/// Return a view to all coordinates of the dataset.
Coords &Dataset::coords() noexcept { return m_coords; }

/// Alias for coords().
const Coords &Dataset::meta() const noexcept { return coords(); }
/// Alias for coords().
Coords &Dataset::meta() noexcept { return coords(); }

bool Dataset::contains(const std::string &name) const noexcept {
  return m_data.count(name) == 1;
}

/// Removes a data item from the Dataset
///
/// Coordinates are not modified.
void Dataset::erase(const std::string &name) {
  expect_writable(*this);
  scipp::expect::contains(*this, name);
  m_data.erase(std::string(name));
  rebuildDims();
}

/// Extract a data item from the Dataset, returning a DataArray
///
/// Coordinates are not modified.
DataArray Dataset::extract(const std::string &name) {
  auto extracted = operator[](name);
  erase(name);
  return extracted;
}

/// Return a data item with coordinates with given name.
DataArray Dataset::operator[](const std::string &name) const {
  scipp::expect::contains(*this, name);
  return *find(name);
}

/// Consistency-enforcing update of the dimensions of the dataset.
///
/// Calling this in the various set* methods prevents insertion of variables
/// with bad shape. This supports insertion of bin edges. Note that the current
/// implementation does not support shape-changing operations which would in
/// theory be permitted but are probably not important in reality: The previous
/// extent of a replaced item is not excluded from the check, so even if that
/// replaced item is the only one in the dataset with that dimension it cannot
/// be "resized" in this way.
void Dataset::setSizes(const Sizes &sizes) {
  m_coords.setSizes(merge(m_coords.sizes(), sizes));
}

void Dataset::rebuildDims() {
  m_coords.rebuildSizes();
  for (const auto &d : *this)
    setSizes(d.dims());
}

/// Set (insert or replace) the coordinate for the given dimension.
void Dataset::setCoord(const Dim dim, Variable coord) {
  expect_writable(*this);
  bool set_sizes = true;
  for (const auto &coord_dim : coord.dims())
    if (is_edges(m_coords.sizes(), coord.dims(), coord_dim))
      set_sizes = false;
  if (set_sizes)
    setSizes(coord.dims());
  m_coords.set(dim, std::move(coord));
}

/// Set (insert or replace) data (values, optional variances) with given name.
///
/// Throws if the provided values bring the dataset into an inconsistent state
/// (mismatching dimensions). The default is to drop existing attributes, unless
/// AttrPolicy::Keep is specified.
void Dataset::setData(const std::string &name, Variable data,
                      const AttrPolicy attrPolicy) {
  expect_writable(*this);
  setSizes(data.dims());
  const auto replace = contains(name);
  if (replace && attrPolicy == AttrPolicy::Keep)
    m_data[name] = DataArray(data, {}, m_data[name].masks().items(),
                             m_data[name].attrs().items(), name);
  else
    m_data[name] = DataArray(data);
  if (replace)
    rebuildDims();
}

/// Set (insert or replace) data from a DataArray with a given name.
///
/// Coordinates, masks, and attributes of the data array are added to the
/// dataset. Throws if there are existing but mismatching coords, masks, or
/// attributes. Throws if the provided data brings the dataset into an
/// inconsistent state (mismatching dimensions).
void Dataset::setData(const std::string &name, const DataArray &data) {
  // Return early on self assign to avoid exceptions from Python inplace ops
  if (const auto it = find(name); it != end()) {
    if (it->data().is_same(data.data()) && it->masks() == data.masks() &&
        it->attrs() == data.attrs() && it->coords() == data.coords())
      return;
  }
  expect_writable(*this);
  for (auto &&[dim, coord] : data.coords())
    if (const auto it = m_coords.find(dim); it != m_coords.end())
      expect::matching_coord(dim, coord, it->second, "set coord");
  setSizes(data.dims());
  for (auto &&[dim, coord] : data.coords())
    if (const auto it = m_coords.find(dim); it == m_coords.end())
      setCoord(dim, std::move(coord));

  setData(name, std::move(data.data()));
  auto &item = m_data[name];

  for (auto &&[dim, attr] : data.attrs())
    // Attrs might be shadowed by a coord, but this cannot be prevented in
    // general, so instead of failing here we proceed (and may fail later if
    // meta() is called).
    item.attrs().set(dim, std::move(attr));
  for (auto &&[nm, mask] : data.masks())
    item.masks().set(nm, std::move(mask));
}

/// Return slice of the dataset along given dimension with given extents.
Dataset Dataset::slice(const Slice &s) const {
  Dataset out;
  out.m_data = slice_map(m_coords.sizes(), m_data, s);
  auto [coords, attrs] = m_coords.slice_coords(s);
  out.m_coords = std::move(coords);
  for (auto &item : out.m_data) {
    Attrs item_attrs(out.m_coords.sizes(), {});
    for (const auto &[dim, coord] : attrs)
      if (m_coords.item_applies_to(dim, m_data.at(item.first).dims()))
        item_attrs.set(dim, coord.as_const());
    item.second.attrs() = item.second.attrs().merge_from(item_attrs);
  }
  out.m_readonly = true;
  return out;
}

Dataset &Dataset::setSlice(const Slice &s, const Dataset &data) {
  // Validate slice on all items as a dry-run
  expect::coords_are_superset(slice(s).coords(), data.coords(), "");
  for (const auto &[name, item] : m_data)
    item.validateSlice(s, data.m_data.at(name));
  // Only if all items checked for dry-run does modification go-ahead
  for (auto &[name, item] : m_data)
    item.setSlice(s, data.m_data.at(name));
  return *this;
}

Dataset &Dataset::setSlice(const Slice &s, const DataArray &data) {
  // Validate slice on all items as a dry-run
  expect::coords_are_superset(slice(s).coords(), data.coords(), "");
  for (const auto &item : m_data)
    item.second.validateSlice(s, data);
  // Only if all items checked for dry-run does modification go-ahead
  for (auto &item : m_data)
    item.second.setSlice(s, data);
  return *this;
}

Dataset &Dataset::setSlice(const Slice &s, const Variable &data) {
  for (auto &&item : *this)
    item.setSlice(s, data);
  return *this;
}

/// Rename dimension `from` to `to`.
void Dataset::rename(const Dim from, const Dim to) {
  if ((from != to) && m_coords.sizes().contains(to))
    throw except::DimensionError("Duplicate dimension.");
  m_coords.rename(from, to);
  for (auto &item : m_data)
    if (item.second.dims().contains(from))
      item.second.rename(from, to);
}

/// Return true if the datasets have identical content.
bool Dataset::operator==(const Dataset &other) const {
  if (size() != other.size())
    return false;
  if (coords() != other.coords())
    return false;
  for (const auto &data : *this)
    if (!other.contains(data.name()) || data != other[data.name()])
      return false;
  return true;
}

/// Return true if the datasets have mismatching content./
bool Dataset::operator!=(const Dataset &other) const {
  return !operator==(other);
}

bool equals_nan(const Dataset &a, const Dataset &b) {
  if (a.size() != b.size())
    return false;
  if (!equals_nan(a.coords(), b.coords()))
    return false;
  for (const auto &data : a)
    if (!b.contains(data.name()) || !equals_nan(data, b[data.name()]))
      return false;
  return true;
}

const Sizes &Dataset::sizes() const { return m_coords.sizes(); }
const Sizes &Dataset::dims() const { return sizes(); }
Dim Dataset::dim() const {
  core::expect::ndim_is(sizes(), 1);
  return *sizes().begin();
}
scipp::index Dataset::ndim() const { return scipp::size(m_coords.sizes()); }

bool Dataset::is_readonly() const noexcept { return m_readonly; }

typename Masks::holder_type union_or(const Masks &currentMasks,
                                     const Masks &otherMasks) {
  typename Masks::holder_type out;
  for (const auto &[key, item] : currentMasks)
    out.emplace(key, copy(item));
  for (const auto &[key, item] : otherMasks) {
    const auto it = currentMasks.find(key);
    if (it == currentMasks.end())
      out.emplace(key, copy(item));
    else if (out[key].dims().includes(item.dims()))
      out[key] |= item;
    else
      out[key] = out[key] | item;
  }
  return out;
}

void union_or_in_place(Masks &masks, const Masks &otherMasks) {
  using core::to_string;
  using units::to_string;
  for (const auto &[key, item] : otherMasks) {
    const auto it = masks.find(key);
    if (it == masks.end() && masks.is_readonly()) {
      throw except::NotFoundError("Cannot insert new mask '" + to_string(key) +
                                  "' via a slice.");
    } else if (it != masks.end() && it->second.is_readonly() &&
               it->second != (it->second | item)) {
      throw except::DimensionError("Cannot update mask '" + to_string(key) +
                                   "' via slice since the mask is implicitly "
                                   "broadcast along the slice dimension.");
    }
  }
  for (const auto &[key, item] : otherMasks) {
    const auto it = masks.find(key);
    if (it == masks.end()) {
      masks.set(key, copy(item));
    } else if (!it->second.is_readonly()) {
      it->second |= item;
    }
  }
}

} // namespace scipp::dataset
