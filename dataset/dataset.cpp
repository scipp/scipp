// SPDX-License-Identifier: GPL-3.0-or-later
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
  if (m_data.erase(std::string(name)) == 0) {
    throw except::NotFoundError("Expected " + to_string(*this) +
                                " to contain " + name + ".");
  }
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

// TODO should we just not supported "shrinking"? That is, what is set first
// determines dims? How can we support empty datasets to carry only bin edges?
namespace extents {
// Internally use negative extent -1 to indicate unknown edge state. The `-1`
// is required for dimensions with extent 0.
scipp::index makeUnknownEdgeState(const scipp::index extent) {
  return -extent - 1;
}
bool isUnknownEdgeState(const scipp::index extent) { return extent < 0; }
scipp::index decode(const scipp::index extent) {
  if (isUnknownEdgeState(extent))
    return -extent - 1;
  return extent;
}
void setExtent(std::unordered_map<Dim, scipp::index> &dims, const Dim dim,
               const scipp::index extent, const bool isCoord) {
  const auto it = dims.find(dim);
  if (it == dims.end()) {
    dims[dim] = isCoord ? extents::makeUnknownEdgeState(extent) : extent;
    return;
  }
  auto &current = it->second;
  if ((extent == decode(current) && !isCoord) ||
      (extent == decode(current) + 1 && isCoord))
    current = decode(current); // switch to known
  if (extent == decode(current) - 1 && isUnknownEdgeState(current))
    current = extent; // shrink by 1 and switch to known
  if (extent != decode(current) && !(isCoord && extent == decode(current) + 1))
    throw except::DimensionError(decode(current), extent);
}
} // namespace extents

/// Consistency-enforcing update of the dimensions of the dataset.
///
/// Calling this in the various set* methods prevents insertion of variable with
/// bad shape. This supports insertion of bin edges. Note that the current
/// implementation does not support shape-changing operations which would in
/// theory be permitted but are probably not important in reality: The previous
/// extent of a replaced item is not excluded from the check, so even if that
/// replaced item is the only one in the dataset with that dimension it cannot
/// be "resized" in this way.
void Dataset::setDims(const Dimensions &dims, const Dim coordDim) {
  auto tmp = m_dims;
  for (const auto &dim : dims.labels())
    extents::setExtent(tmp, dim, dims[dim], dim == coordDim);
  m_dims = tmp;
  m_coords.sizes() = Sizes(dimensions());
}

void Dataset::rebuildDims() {
  m_dims.clear();

  for (const auto &d : *this) {
    setDims(d.dims());
  }
  for (const auto &c : m_coords) {
    setDims(c.second.dims(), dim_of_coord(c.second, c.first));
  }
}

/// Set (insert or replace) the coordinate for the given dimension.
void Dataset::setCoord(const Dim dim, Variable coord) {
  // ds.coords().set() must be possible (in Python?)
  // ... how can we allow for growing sizes?
  setDims(coord.dims(), dim_of_coord(coord, dim));
  // TODO remove?
  // for (const auto &item : m_data)
  //  if (item.second.coords.count(dim))
  //    throw except::DataArrayError("Attempt to insert dataset coord with "
  //                                 "name " +
  //                                 to_string(dim) + "  shadowing attribute.");
  m_coords.set(dim, std::move(coord));
}

/// Set (insert or replace) data (values, optional variances) with given name.
///
/// Throws if the provided values bring the dataset into an inconsistent state
/// (mismatching dtype, unit, or dimensions). The default is to drop existing
/// attributes, unless AttrPolicy::Keep is specified.
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

/// Set (insert or replace) data from a DataArray with a given name, avoiding
/// copies where possible by using std::move. TODO move does not make sense
///
/// Coordinates, masks, and attributes of the data array are added to the
/// dataset. Throws if there are existing but mismatching coords, masks, or
/// attributes. Throws if the provided data brings the dataset into an
/// inconsistent state (mismatching dtype, unit, or dimensions).
void Dataset::setData(const std::string &name, const DataArray &data) {
  // TODO
  // if (contains(name) && &m_data[name] == &data.underlying() &&
  //    data.slices().empty())
  //  return; // Self-assignment, return early.
  // Sizes new_sizes(data.dims());
  // TODO
  // no... what if item replace shrinks sizes
  // new_sizes = merge(m_sizes, sizes);

  for (auto &&[dim, coord] : data.coords()) {
    if (const auto it = m_coords.find(dim); it != m_coords.end())
      core::expect::equals(coord, it->second);
    else
      setCoord(dim, std::move(coord));
  }

  setData(name, std::move(data.data()));
  auto &item = m_data[name];

  for (auto &&[dim, attr] : item.attrs())
    // TODO dropping not really necessary in new mechanism, fail later
    // Drop unaligned coords if there is aligned coord with same name.
    if (!coords().contains(dim))
      item.attrs().set(dim, std::move(attr));
  for (auto &&[nm, mask] : data.masks())
    item.masks().set(nm, std::move(mask));
}

/// Return slice of the dataset along given dimension with given extents.
Dataset Dataset::slice(const Slice s) const {
  Dataset out;
  // TODO m_dims
  out.m_coords = m_coords.slice(s);
  // TODO drop items that do not depend on s.dim()?
  out.m_data = slice_map(m_coords.sizes(), m_data, s);
  for (auto it = out.m_coords.begin(); it != out.m_coords.end();) {
    if (unaligned_by_dim_slice(*it, s.dim())) {
      for (auto &item : out.m_data)
        item.second.attrs().set(it->first, it->second);
      out.m_coords.erase(it->first);
    }
    ++it;
  }
  return out;
}

/// Rename dimension `from` to `to`.
void Dataset::rename(const Dim from, const Dim to) {
  if ((from != to) && (m_dims.count(to) != 0))
    throw except::DimensionError("Duplicate dimension.");

  const auto relabel = [from, to](auto &map) {
    auto node = map.extract(from);
    node.key() = to;
    map.insert(std::move(node));
  };
  if (m_dims.count(from) != 0)
    relabel(m_dims);
  if (m_coords.count(from))
    relabel(m_coords.items());
  for (auto &item : m_coords)
    item.second.rename(from, to);
  for (auto &item : m_data)
    item.second.rename(from, to);
}

namespace {

template <class T> const auto &getitem(const T &view, const std::string &name) {
  if (auto it = view.find(name); it != view.end())
    return *it;
  throw except::NotFoundError("Expected " + to_string(view) + " to contain " +
                              name + ".");
}
} // namespace

/*
// This is a member so it gets access to a private constructor of DataArrayView.
template <class T>
std::pair<boost::container::small_vector<DataArrayView, 8>, detail::slice_list>
DatasetConstView::slice_items(const T &view, const Slice slice) {
  auto slices = view.slices();
  boost::container::small_vector<DataArrayView, 8> items;
  scipp::index extent = std::numeric_limits<scipp::index>::max();
  for (const auto &item : view) {
    const auto &dims = item.dims();
    if (dims.contains(slice.dim())) {
      items.emplace_back(DataArrayView(item.slice(slice)));
      // In principle data may be on bin edges. The overall dimension is then
      // determined by the extent of data that is *not* on the edges.
      extent = std::min(extent, dims[slice.dim()]);
    }
  }
  if (extent == std::numeric_limits<scipp::index>::max()) {
    // Fallback: Could not determine extent from data (not data that depends on
    // slicing dimension), use `dimensions()` to also consider coords.
    const auto currentDims = view.dimensions();
    core::expect::validSlice(currentDims, slice);
    extent = currentDims.at(slice.dim());
  }
  slices.emplace_back(slice, extent);
  return std::pair{std::move(items), std::move(slices)};
}

/// Return a slice of the dataset view.
///
/// The returned view will not contain references to data items that do not
/// depend on the sliced dimension.
DatasetConstView DatasetConstView::slice(const Slice s) const {
  DatasetConstView sliced;
  sliced.m_dataset = m_dataset;
  std::tie(sliced.m_items, sliced.m_slices) = slice_items(*this, s);
  return sliced;
}
*/

template <class A, class B> bool dataset_equals(const A &a, const B &b) {
  if (a.size() != b.size())
    return false;
  if (a.coords() != b.coords())
    return false;
  for (const auto &data : a)
    if (!b.contains(data.name()) || data != b[data.name()])
      return false;
  return true;
}

/// Return true if the datasets have identical content.
bool Dataset::operator==(const Dataset &other) const {
  return dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content./
bool Dataset::operator!=(const Dataset &other) const {
  return !operator==(other);
}

std::unordered_map<Dim, scipp::index> Dataset::dimensions() const {
  std::unordered_map<Dim, scipp::index> all;
  for (const auto &dim : this->m_dims)
    all[dim.first] = extents::decode(dim.second);
  return all;
}

std::unordered_map<typename Masks::key_type, typename Masks::mapped_type>
union_or(const Masks &currentMasks, const Masks &otherMasks) {
  std::unordered_map<typename Masks::key_type, typename Masks::mapped_type> out;

  for (const auto &[key, item] : currentMasks) {
    out.emplace(key, item);
  }

  for (const auto &[key, item] : otherMasks) {
    const auto it = currentMasks.find(key);
    if (it != currentMasks.end()) {
      out[key] |= item;
    } else {
      out.emplace(key, item);
    }
  }
  return out;
}

void union_or_in_place(Masks &currentMasks, const Masks &otherMasks) {
  for (const auto &[key, item] : otherMasks) {
    const auto it = currentMasks.find(key);
    if (it != currentMasks.end()) {
      it->second |= item;
    } else {
      currentMasks.set(key, item);
    }
  }
}

void copy_metadata(const DataArray &a, DataArray &b) {
  copy_items(a.coords(), b.coords());
  copy_items(a.masks(), b.masks());
  copy_items(a.attrs(), b.attrs());
}

} // namespace scipp::dataset
