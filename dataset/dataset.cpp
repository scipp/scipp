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

template <class T> typename T::view_type makeViewItem(T &variable) {
  if constexpr (std::is_const_v<T>)
    return typename T::view_type(typename T::const_view_type(variable));
  else
    return typename T::view_type(variable);
}

template <class T1> auto makeViewItems(T1 &coords) {
  std::unordered_map<typename T1::key_type, VariableView> items;
  for (auto &item : coords)
    items.emplace(item.first, makeViewItem(item.second));
  return items;
}

template <class Dims, class T1>
auto makeViewItems(const Dims &dims, T1 &coords) {
  std::unordered_map<typename T1::key_type, VariableView> items;
  // We preserve only items that are part of the space spanned by the
  // provided parent dimensions.
  auto contained = [&dims](const auto &coord) {
    for (const Dim &dim : coord.second.dims().labels())
      if (dims.count(dim) == 0)
        return false;
    return true;
  };
  for (auto &item : coords)
    if (contained(item))
      items.emplace(item.first, makeViewItem(item.second));
  return items;
}

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
  setDims(coord.dims(), dim_of_coord(coord, dim));
  for (const auto &item : m_data)
    if (item.second.coords.count(dim))
      throw except::DataArrayError("Attempt to insert dataset coord with "
                                   "name " +
                                   to_string(dim) + "  shadowing attribute.");
  m_coords.set(dim, std::move(coord));
}

/// Set (insert or replace) an attribute for item with given name.
void Dataset::setCoord(const std::string &name, const Dim dim, Variable coord) {
  scipp::expect::contains(*this, name);
  if (coords().contains(dim))
    throw except::DataArrayError("Attempt to insert attribute with name " +
                                 to_string(dim) + " shadowing coord.");
  setDims(coord.dims(), dim_of_coord(coord, dim));
  m_data[name].attrs().set(dim, std::move(coord));
}

/// Set (insert or replace) an mask for item with given name.
void Dataset::setMask(const std::string &name, const std::string &maskName,
                      Variable mask) {
  scipp::expect::contains(*this, name);
  setDims(mask.dims());
  m_data[name].masks().set(maskName, std::move(mask));
}

void Dataset::setData_impl(const std::string &name, detail::DatasetData &&data,
                           const AttrPolicy attrPolicy) {
  setDims(data.data.dims());
  const auto replace = contains(name);
  if (replace && attrPolicy == AttrPolicy::Keep)
    data.coords = std::move(m_data[name].coords);
  m_data[name] = std::move(data);
  if (replace)
    rebuildDims();
}

/// Set (insert or replace) data (values, optional variances) with given name.
///
/// Throws if the provided values bring the dataset into an inconsistent state
/// (mismatching dtype, unit, or dimensions). The default is to drop existing
/// attributes, unless AttrPolicy::Keep is specified.
void Dataset::setData(const std::string &name, Variable data,
                      const AttrPolicy attrPolicy) {
  setData_impl(name, detail::DatasetData{std::move(data), {}, {}}, attrPolicy);
}

/// Set (insert or replace) data from a DataArray with a given name, avoiding
/// copies where possible by using std::move.
void Dataset::setData(const std::string &name, DataArray data) {
  // Get the Dataset holder
  auto dataset = DataArray::to_dataset(std::move(data));
  // There can be only one DatasetData item, so get the first one with begin()
  auto item = dataset.m_data.begin();

  for (auto &&[dim, coord] : dataset.m_coords) {
    if (const auto it = m_coords.find(dim); it != m_coords.end())
      core::expect::equals(coord, it->second);
    else
      setCoord(dim, std::move(coord));
  }

  setData(name, std::move(item->second.data));

  for (auto &&[dim, coord] : item->second.coords)
    // Drop unaligned coords if there is aligned coord with same name.
    if (!coords().contains(dim))
      setCoord(name, dim, std::move(coord));
  for (auto &&[nm, mask] : item->second.masks)
    setMask(name, std::string(nm), std::move(mask));
}

/// Set (insert or replace) data item with given name.
///
/// Coordinates, masks, and attributes of the data array are added to the
/// dataset. Throws if there are existing but mismatching coords, masks, or
/// attributes. Throws if the provided data brings the dataset into an
/// inconsistent state (mismatching dtype, unit, or dimensions).
void Dataset::setData(const std::string &name, const DataArray &data) {
  if (contains(name) && &m_data[name] == &data.underlying() &&
      data.slices().empty())
    return; // Self-assignment, return early.
  setData(name, DataArray(data));
}

void DataArrayView::setData(Variable data) const {
  if (!slices().empty())
    throw except::SliceError("Cannot set data via slice.");
  m_mutableDataset->setData(name(), std::move(data), AttrPolicy::Keep);
}

template <class Key, class Val>
Val Dataset::extract_from_map(std::unordered_map<Key, Val> &map,
                              const Key &key) {
  using core::to_string;
  if (!map.count(key))
    throw except::NotFoundError("Cannot erase " + to_string(key) +
                                " -- not found.");
  auto value = std::move(map.at(key));
  map.erase(key);
  rebuildDims();
  return value;
}

/// Remove and return the coordinate for the given dimension.
Variable Dataset::extractCoord(const Dim dim) {
  return extract_from_map(m_coords, dim);
}

/// Remove and return unaligned coord with given name from the given item.
Variable Dataset::extractCoord(const std::string &name, const Dim dim) {
  scipp::expect::contains(*this, name);
  return extract_from_map(m_data[name].coords, dim);
}

/// Remove and return mask with given mask name from the given item.
Variable Dataset::extractMask(const std::string &name,
                              const std::string &maskName) {
  scipp::expect::contains(*this, name);
  return extract_from_map(m_data[name].masks, maskName);
}

/// Return slice of the dataset along given dimension with given extents.
DatasetC Dataset::slice(const Slice s) const {
  Dataset out;
  // TODO m_dims
  // TODO coord attr conversion not possible for dataset, but how can we handle
  // it for items? Can it be done based on dims?
  out.m_coords = m_coords.slice(s);
  out.m_data = slice_map(m_data, s);
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
    relabel(m_coords);
  for (auto &item : m_coords)
    item.second.rename(from, to);
  for (auto &item : m_data) {
    auto &value = item.second;
    value.data.rename(from, to);
    for (auto &coord : value.coords)
      coord.second.rename(from, to);
    for (auto &mask : value.masks)
      mask.second.rename(from, to);
  }
}

namespace {
auto unaligned_by_dim_slice = [](const auto &item, const Dim dim) {
  const auto &[key, var] = item;
  if constexpr (std::is_same_v<std::decay_t<decltype(item.first)>, Dim>) {
    const bool is_dimension_coord = var.dims().contains(key);
    return var.dims().contains(dim) &&
           (is_dimension_coord ? key == dim : var.dims().inner() == dim);
  } else {
    return false;
  }
};

template <class Items>
void erase_if_unaligned_by_dim_slice(Items &items, const Dim dim) {
  for (auto it = items.begin(); it != items.end();) {
    if (unaligned_by_dim_slice(*it, dim))
      it = items.erase(it);
    else
      ++it;
  }
}

template <class Items, class Slices>
void erase_if_unaligned_by_dim_slices(Items &items, const Slices &slices) {
  for (const auto &slice : slices)
    if (!slice.first.isRange())
      erase_if_unaligned_by_dim_slice(items, slice.first.dim());
}

template <class Items, class Slices>
void keep_if_unaligned_by_dim_slices(Items &items, const Slices &slices) {
  constexpr auto keep = [](const auto &item, const auto &slices2) {
    for (const auto &slice : slices2)
      if (!slice.first.isRange() &&
          unaligned_by_dim_slice(item, slice.first.dim()))
        return true;
    return false;
  };
  for (auto it = items.begin(); it != items.end();) {
    if (keep(*it, slices))
      ++it;
    else
      it = items.erase(it);
  }
}

/// Conditionally removes coords from view to implement mapping of aligned
/// coords to unaligned coords for non-range slices of data arrays or datasets.
template <class Items, class Slices>
void maybe_drop_aligned_or_unaligned(Items &items, const Slices &slices,
                                     const CoordCategory category) {
  if (category == CoordCategory::Aligned)
    erase_if_unaligned_by_dim_slices(items, slices);
  if (category == CoordCategory::Unaligned)
    keep_if_unaligned_by_dim_slices(items, slices);
}
} // namespace

/// is_item controls whether access should refer to aligned or unaligned coords,
/// i.e., whether this is a view of a dataset item or a data array.
template <class T>
std::conditional_t<std::is_same_v<T, DataArrayView>, CoordsView,
                   CoordsConstView>
make_coords(const T &view, const CoordCategory category,
            [[maybe_unused]] const bool is_item) {
  // Aligned coords (including unaligned by slicing) from dataset. This is all
  // coords of the dataset *excluding* those that exceed the item dims.
  auto items = makeViewItems(view.parentDims(), view.get_dataset().m_coords);
  maybe_drop_aligned_or_unaligned(items, view.slices(), category);
  const bool aligned = !(category & CoordCategory::Unaligned);
  if (category & CoordCategory::Unaligned) {
    // Unaligned coords: Need to include everything, in particular to preserve
    // bin edges after non-range slice.
    const auto tmp = makeViewItems(view.get_data().coords);
    items.insert(tmp.begin(), tmp.end());
  }
  if constexpr (std::is_same_v<T, DataArrayView>) {
    const bool aligned_of_item = is_item && aligned;
    // insert/erase disabled for `meta`
    const bool combined = category == CoordCategory::All;
    return CoordsView(
        // Coord insert/erase disabled if:
        // - coords of a slice:
        //   array['x', 7].coords['x'] = x # fails
        //   array.coords['x'] = x # ok
        // - (aligned) coords of a dataset item:
        //   del ds['a'].coords['x'] # fails
        //   del ds.coords['x'] # ok
        CoordAccess{(view.slices().empty() && !aligned_of_item && !combined)
                        ? &view.get_dataset()
                        : nullptr,
                    &view.name(), is_item},
        std::move(items), view.slices());
  } else
    return CoordsConstView(std::move(items), view.slices());
}

/// Return a const view to all coordinates of the dataset slice.
CoordsConstView DatasetConstView::coords() const noexcept {
  auto items = makeViewItems(m_dataset->dimensions(), m_dataset->m_coords);
  erase_if_unaligned_by_dim_slices(items, slices());
  return CoordsConstView(std::move(items), slices());
}

/// Return a view to all coordinates of the dataset slice.
CoordsView DatasetView::coords() const noexcept {
  auto items =
      makeViewItems(m_mutableDataset->dimensions(), m_mutableDataset->m_coords);
  erase_if_unaligned_by_dim_slices(items, slices());
  return CoordsView(CoordAccess(slices().empty() ? m_mutableDataset : nullptr),
                    std::move(items), slices());
}

namespace {
template <class T> const auto &getitem(const T &view, const std::string &name) {
  if (auto it = view.find(name); it != view.end())
    return *it;
  throw except::NotFoundError("Expected " + to_string(view) + " to contain " +
                              name + ".");
}
} // namespace

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

DatasetView DatasetView::slice(const Slice s) const {
  DatasetView sliced;
  sliced.m_dataset = m_dataset;
  sliced.m_mutableDataset = m_mutableDataset;
  std::tie(sliced.m_items, sliced.m_slices) = slice_items(*this, s);
  return sliced;
}

template <class A, class B> bool dataset_equals(const A &a, const B &b) {
  if (a.size() != b.size())
    return false;
  if (a.coords() != b.coords())
    return false;
  for (const auto &data : a) {
    try {
      if (data != b[data.name()])
        return false;
    } catch (except::NotFoundError &) {
      return false;
    }
  }
  return true;
}

/// Return true if the datasets have identical content.
bool Dataset::operator==(const Dataset &other) const {
  return dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content./
bool Dataset::operator!=(const Dataset &other) const {
  return !operator==(*this, other);
}

std::unordered_map<Dim, scipp::index> Dataset::dimensions() const {
  std::unordered_map<Dim, scipp::index> all;
  for (const auto &dim : this->m_dims)
    all[dim.first] = extents::decode(dim.second);
  return all;
}

std::map<typename Masks::key_type, typename Masks::mapped_type>
union_or(const Masks &currentMasks, const Masks &otherMasks) {
  std::map<typename Masks::key_type, typename Masks::mapped_type> out;

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
