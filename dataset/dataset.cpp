// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset.h"
#include "scipp/common/index.h"
#include "scipp/core/except.h"
#include "scipp/dataset/except.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

Dataset::Dataset(const DataArray &data) { setData(data.name(), data); }

/// Removes all data items from the Dataset.
///
/// Coordinates are not modified.
void Dataset::clear() {
  m_data.clear();
  rebuildDims();
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
void Dataset::setDims(const Dimensions &dims, const Dim coordDim) {
  if (coordDim != Dim::Invalid && is_edges(m_coords.sizes(), dims, coordDim))
    return;
  m_coords.setSizes(merge(m_coords.sizes(), Sizes(dims)));
}

void Dataset::rebuildDims() {
  m_coords.rebuildSizes();
  for (const auto &d : *this)
    setDims(d.dims());
}

/// Set (insert or replace) the coordinate for the given dimension.
void Dataset::setCoord(const Dim dim, Variable coord) {
  setDims(coord.dims(), dim_of_coord(coord, dim));
  m_coords.set(dim, std::move(coord));
}

/// Set (insert or replace) data (values, optional variances) with given name.
///
/// Throws if the provided values bring the dataset into an inconsistent state
/// (mismatching dimensions). The default is to drop existing attributes, unless
/// AttrPolicy::Keep is specified.
void Dataset::setData(const std::string &name, Variable data,
                      const AttrPolicy attrPolicy) {
  setDims(data.dims());
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
/// inconsistent state (mismatching dtype, unit, or dimensions).
void Dataset::setData(const std::string &name, const DataArray &data) {
  setDims(data.dims());
  for (auto &&[dim, coord] : data.coords()) {
    if (const auto it = m_coords.find(dim); it != m_coords.end())
      core::expect::equals(coord, it->second);
    else
      setCoord(dim, std::move(coord));
  }

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
Dataset Dataset::slice(const Slice s) const {
  Dataset out;
  out.m_coords = m_coords.slice(s);
  out.m_data = slice_map(m_coords.sizes(), m_data, s);
  Attrs out_attrs(out.m_coords.sizes(), {});
  for (auto it = m_coords.begin(); it != m_coords.end(); ++it)
    if (unaligned_by_dim_slice(*it, s))
      out_attrs.set(it->first, out.m_coords.extract(it->first));
  for (auto &item : out.m_data) {
    Attrs item_attrs(out.m_coords.sizes(), {});
    for (const auto &[dim, coord] : out_attrs)
      if (m_coords.item_applies_to(dim, m_data.at(item.first).dims()))
        item_attrs.set(dim, coord.as_const());
    item.second.attrs() = item.second.attrs().merge_from(item_attrs);
  }
  return out;
}

Dataset &Dataset::setSlice(const Slice s, const Dataset &data) {
  // TODO Need dry-run mechanism here?
  for (const auto &item : data)
    (*this)[item.name()].setSlice(s, item);
  return *this;
}

Dataset &Dataset::setSlice(const Slice s, const DataArray &data) {
  // TODO Need dry-run mechanism here?
  for (auto &&item : *this)
    item.setSlice(s, data);
  return *this;
}

Dataset &Dataset::setSlice(const Slice s, const Variable &data) {
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

const Sizes &Dataset::sizes() const { return m_coords.sizes(); }
const Sizes &Dataset::dims() const { return sizes(); }

typename Masks::holder_type union_or(const Masks &currentMasks,
                                     const Masks &otherMasks) {
  typename Masks::holder_type out;
  for (const auto &[key, item] : currentMasks)
    out.emplace(key, copy(item));
  for (const auto &[key, item] : otherMasks) {
    const auto it = currentMasks.find(key);
    if (it == currentMasks.end())
      out.emplace(key, copy(item));
    else if (out[key].dims().contains(item.dims()))
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
