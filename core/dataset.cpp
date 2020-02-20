// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"

namespace scipp::core {

template <class T>
std::pair<const Variable *, Variable *> makeViewItem(T *variable) {
  if constexpr (std::is_const_v<T>)
    return {variable, nullptr};
  else
    return {variable, variable};
}

template <class Key, class T1> auto makeViewItems(T1 &coords) {
  std::unordered_map<Key, std::pair<const Variable *, Variable *>> items;
  for (auto &item : coords)
    items.emplace(item.first, makeViewItem(&item.second));
  return items;
}

template <class Key, class T1, class T2 = void>
auto makeViewItems(const Dimensions &dims, T1 &coords, T2 *sparse = nullptr) {
  const Dim sparseDim = dims.sparseDim();
  std::unordered_map<Key, std::pair<const Variable *, Variable *>> items;
  for (auto &item : coords) {
    // We preserve only items that are part of the space spanned by the
    // provided parent dimensions. Note the use of std::any_of (not
    // std::all_of): At this point there may still be extra dimensions in item,
    // but they will be sliced out. Maybe a better implementation would be to
    // slice the coords first? That would also eliminate a potential loophole
    // for multi-dimensional coordinates.
    auto contained = [&dims](const auto &item2) {
      const auto &coordDims = item2.second.dims();
      if constexpr (std::is_same_v<Key, Dim>) {
        const bool is_dimension_coord = coordDims.contains(item2.first);
        return coordDims.empty() ||
               (is_dimension_coord ? dims.contains(item2.first)
                                   : dims.contains(coordDims.inner()));
      } else
        return coordDims.empty() || dims.contains(coordDims.inner());
    };
    if (contained(item)) {
      // Shadow all global coordinates that depend on the sparse dimension.
      if ((!dims.sparse()) || (!item.second.dims().contains(sparseDim)))
        items.emplace(item.first, makeViewItem(&item.second));
    }
  }
  if (sparse) {
    if constexpr (std::is_same_v<T2, const Variable> ||
                  std::is_same_v<T2, Variable>) {
      items.emplace(sparseDim, makeViewItem(sparse));
    } else if constexpr (!std::is_same_v<T2, void>) {
      for (auto &item : *sparse)
        items.emplace(item.first, makeViewItem(&item.second));
    }
  }
  return items;
}

Dataset::Dataset(const DatasetConstView &view)
    : Dataset(view, view.coords(), view.labels(), view.masks(), view.attrs()) {}

Dataset::Dataset(const DataArrayConstView &data) { setData(data.name(), data); }

Dataset::Dataset(const std::map<std::string, DataArrayConstView> &data) {
  for (const auto &[name, item] : data)
    setData(name, item);
}

/// Removes all data items from the Dataset.
///
/// Coordinates, labels, attributes and masks are not modified.
/// This operation invalidates any view objects creeated from this dataset.
void Dataset::clear() {
  m_data.clear();
  rebuildDims();
}

/// Return a const view to all coordinates of the dataset.
///
/// This view includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsConstView Dataset::coords() const noexcept {
  return CoordsConstView(makeViewItems<Dim>(m_coords));
}

/// Return a view to all coordinates of the dataset.
///
/// This view includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsView Dataset::coords() noexcept {
  return CoordsView(this, nullptr, makeViewItems<Dim>(m_coords));
}

/// Return a const view to all labels of the dataset.
LabelsConstView Dataset::labels() const noexcept {
  return LabelsConstView(makeViewItems<std::string>(m_labels));
}

/// Return a view to all labels of the dataset.
LabelsView Dataset::labels() noexcept {
  return LabelsView(this, nullptr, makeViewItems<std::string>(m_labels));
}

/// Return a const view to all attributes of the dataset.
AttrsConstView Dataset::attrs() const noexcept {
  return AttrsConstView(makeViewItems<std::string>(m_attrs));
}

/// Return a view to all attributes of the dataset.
AttrsView Dataset::attrs() noexcept {
  return AttrsView(this, nullptr, makeViewItems<std::string>(m_attrs));
}

/// Return a const view to all masks of the dataset.
MasksConstView Dataset::masks() const noexcept {
  return MasksConstView(makeViewItems<std::string>(m_masks));
}

/// Return a view to all masks of the dataset.
MasksView Dataset::masks() noexcept {
  return MasksView(this, nullptr, makeViewItems<std::string>(m_masks));
}

bool Dataset::contains(const std::string &name) const noexcept {
  return m_data.count(name) == 1;
}

/// Removes a data item from the Dataset
///
/// Coordinates, labels and attributes are not modified.
/// This operation invalidates any view objects creeated from this dataset.
void Dataset::erase(const std::string_view name) {
  if (m_data.erase(std::string(name)) == 0) {
    throw except::DatasetError(*this, "Could not find data with name " +
                                          std::string(name) + ".");
  }
  rebuildDims();
}

/// Return a const view to data and coordinates with given name.
DataArrayConstView Dataset::operator[](const std::string &name) const {
  expect::contains(*this, name);
  return DataArrayConstView(*this, *m_data.find(name));
}

/// Return a view to data and coordinates with given name.
DataArrayView Dataset::operator[](const std::string &name) {
  expect::contains(*this, name);
  return DataArrayView(*this, *m_data.find(name));
}

namespace extents {
// Internally use negative extent -1 to indicate unknown edge state. The `-1`
// is required for dimensions with extent 0.
scipp::index makeUnknownEdgeState(const scipp::index extent) {
  if (extent == Dimensions::Sparse)
    return extent;
  return -extent - 1;
}
scipp::index shrink(const scipp::index extent) { return extent - 1; }
bool isUnknownEdgeState(const scipp::index extent) {
  return extent < 0 && extent != Dimensions::Sparse;
}
scipp::index decodeExtent(const scipp::index extent) {
  if (isUnknownEdgeState(extent))
    return -extent - 1;
  return extent;
}
bool isSame(const scipp::index extent, const scipp::index reference) {
  return reference == -extent - 1;
}
bool oneLarger(const scipp::index extent, const scipp::index reference) {
  return extent == -reference - 1 + 1;
}
bool oneSmaller(const scipp::index extent, const scipp::index reference) {
  return extent == -reference - 1 - 1;
}
void setExtent(std::unordered_map<Dim, scipp::index> &dims, const Dim dim,
               const scipp::index extent, const bool isCoord) {
  const auto it = dims.find(dim);
  // Internally use negative extent -1 to indicate unknown edge state. The `-1`
  // is required for dimensions with extent 0.
  if (it == dims.end()) {
    dims[dim] = extents::makeUnknownEdgeState(extent);
  } else if (extent != Dimensions::Sparse) {
    auto &heldExtent = it->second;
    if (heldExtent == Dimensions::Sparse) {
      heldExtent = extents::makeUnknownEdgeState(extent);
    }
    if (extents::isUnknownEdgeState(heldExtent)) {
      if (extents::isSame(extent, heldExtent)) { // Do nothing
      } else if (extents::oneLarger(extent, heldExtent) && isCoord) {
        heldExtent = extents::shrink(extent);
      } else if (extents::oneSmaller(extent, heldExtent) && !isCoord) {
        heldExtent = extent;
      } else {
        throw std::runtime_error("Length mismatch on insertion");
      }
    } else {
      // Check for known edge state
      if ((extent != heldExtent || isCoord) && extent != heldExtent + 1)
        throw std::runtime_error("Length mismatch on insertion");
    }
  }
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
  for (const auto dim : dims.denseLabels())
    extents::setExtent(tmp, dim, dims[dim], dim == coordDim);
  if (dims.sparse())
    extents::setExtent(tmp, dims.sparseDim(), Dimensions::Sparse, false);
  m_dims = tmp;
}

void Dataset::rebuildDims() {
  /* Clear old extents */
  m_dims.clear();

  for (const auto &d : *this) {
    setDims(d.dims());
  }
  for (const auto &c : m_coords) {
    setDims(c.second.dims(), c.first);
  }
  for (const auto &l : m_labels) {
    setDims(l.second.dims());
  }
  for (const auto &m : m_masks) {
    setDims(m.second.dims());
  }
  for (const auto &a : m_attrs) {
    setDims(a.second.dims());
  }
}

/// Set (insert or replace) the coordinate for the given dimension.
void Dataset::setCoord(const Dim dim, Variable coord) {
  expect::notSparse(coord);
  setDims(coord.dims(), dim);
  m_coords.insert_or_assign(dim, std::move(coord));
}

/// Set (insert or replace) the labels for the given label name.
///
/// Note that the label name has no relation to names of data items.
void Dataset::setLabels(const std::string &labelName, Variable labels) {
  expect::notSparse(labels);
  setDims(labels.dims());
  m_labels.insert_or_assign(labelName, std::move(labels));
}

/// Set (insert or replace) an attribute for the given attribute name.
///
/// Note that the attribute name has no relation to names of data items.
void Dataset::setAttr(const std::string &attrName, Variable attr) {
  expect::notSparse(attr);
  setDims(attr.dims());
  m_attrs.insert_or_assign(attrName, std::move(attr));
}

/// Set (insert or replace) an attribute for item with given name.
void Dataset::setAttr(const std::string &name, const std::string &attrName,
                      Variable attr) {
  expect::contains(*this, name);
  expect::notSparse(attr);
  if (!operator[](name).dims().contains(attr.dims()))
    throw except::DimensionError(
        "Attribute dimensions must match and not exceed dimensions of data.");
  m_data[name].attrs.insert_or_assign(attrName, std::move(attr));
}

/// Set (insert or replace) the masks for the given mask name.
///
/// Note that the mask name has no relation to names of data items.
void Dataset::setMask(const std::string &masksName, Variable mask) {
  expect::notSparse(mask);
  setDims(mask.dims());
  m_masks.insert_or_assign(masksName, std::move(mask));
}

/// Set (insert or replace) data (values, optional variances) with given name.
///
/// Throws if the provided values bring the dataset into an inconsistent state
/// (mismatching dtype, unit, or dimensions).
void Dataset::setData(const std::string &name, Variable data) {
  setDims(data.dims());
  const bool sparseData = data.dims().sparse();

  if (contains(name) && operator[](name).dims().sparse() != sparseData)
    throw except::DimensionError("Cannot set dense values or variances if "
                                 "coordinates sparse or vice versa");

  const auto rebuild_dims = contains(name);
  m_data[name].data = std::move(data);
  if (rebuild_dims)
    rebuildDims();
}

/// Set (insert or replace) data from a DataArray with a given name, avoiding
/// copies where possible by using std::move.
///
void Dataset::setData(const std::string &name, DataArray data) {
  // Get the Dataset holder
  auto dataset = DataArray::to_dataset(std::move(data));

  for (auto &&[dim, coord] : dataset.m_coords) {
    if (const auto it = m_coords.find(dim); it != m_coords.end())
      expect::equals(coord, it->second);
    else
      setCoord(dim, std::move(coord));
  }
  for (auto &&[nm, labs] : dataset.m_labels) {
    if (const auto it = m_labels.find(std::string(nm)); it != m_labels.end())
      expect::equals(labs, it->second);
    else
      setLabels(std::string(nm), std::move(labs));
  }

  for (auto &&[nm, mask] : dataset.m_masks)
    if (const auto it = m_masks.find(std::string(nm)); it != m_masks.end())
      expect::equals(mask, it->second);
    else
      setMask(std::string(nm), std::move(mask));

  if (dataset.m_attrs.size() > 0)
    throw except::SizeError("Attributes should be empty for a DataArray.");

  // There can be only one DatasetData item, so get the first one with begin()
  auto item = dataset.m_data.begin();
  if (item->second.data)
    setData(name, std::move(item->second.data.value()));
  if (item->second.coord)
    setSparseCoord(name, std::move(item->second.coord.value()));
  for (auto &&[nm, labs] : item->second.labels)
    setSparseLabels(name, std::string(nm), std::move(labs));
  for (auto &&[nm, attr] : item->second.attrs)
    setAttr(name, std::string(nm), std::move(attr));
}

/// Set (insert or replace) data item with given name.
///
/// Coordinates, labels, attributes and masks of the data array are added to the
/// dataset. Throws if there are existing but mismatching coords, labels, or
/// attributes. Throws if the provided data brings the dataset into an
/// inconsistent state (mismatching dtype, unit, or dimensions).
void Dataset::setData(const std::string &name, const DataArrayConstView &data) {
  if (contains(name) && &m_data[name] == &data.underlying() &&
      data.slices().empty())
    return; // Self-assignment, return early.
  setData(name, DataArray(data));
}

/// Set (insert or replace) the sparse coordinate with given name.
///
/// Sparse coordinates can exist even without corresponding data.
void Dataset::setSparseCoord(const std::string &name, Variable coord) {
  if (!coord.dims().sparse())
    throw except::DimensionError(
        "Variable passed to Dataset::setSparseCoord does "
        "not contain sparse data.");
  if (m_data.count(name)) {
    const auto &data = m_data.at(name);
    if ((data.data &&
         (data.data->dims().sparseDim() != coord.dims().sparseDim())) ||
        (!data.labels.empty() &&
         (data.labels.begin()->second.dims().sparseDim() !=
          coord.dims().sparseDim())))
      throw except::DimensionError("Cannot set sparse coordinate if values or "
                                   "variances are not sparse.");
  }
  setDims(coord.dims());
  m_data[name].coord = std::move(coord);
}

/// Set (insert or replace) the sparse labels with given name and label name.
void Dataset::setSparseLabels(const std::string &name,
                              const std::string &labelName, Variable labels) {
  if (!labels.dims().sparse())
    throw std::runtime_error("Variable passed to Dataset::setSparseLabels does "
                             "not contain sparse data.");
  if (m_data.count(name)) {
    const auto &data = m_data.at(name);
    if ((data.data &&
         (data.data->dims().sparseDim() != labels.dims().sparseDim())) ||
        (data.coord &&
         (data.coord->dims().sparseDim() != labels.dims().sparseDim())))
      throw std::runtime_error("Cannot set sparse labels if values or "
                               "variances are not sparse.");
  }

  const auto &data = m_data.at(name);
  if (!data.data && !data.coord)
    throw std::runtime_error(
        "Cannot set sparse labels: Require either values or a sparse coord.");

  setDims(labels.dims());
  m_data[name].labels.insert_or_assign(labelName, std::move(labels));
}

/// Removes the coordinate for the given dimension.
void Dataset::eraseCoord(const Dim dim) { erase_from_map(m_coords, dim); }

/// Removes the labels for the given label name.
void Dataset::eraseLabels(const std::string &labelName) {
  erase_from_map(m_labels, labelName);
}

/// Removes an attribute for the given attribute name.
void Dataset::eraseAttr(const std::string &attrName) {
  erase_from_map(m_attrs, attrName);
}

/// Removes attribute with given attribute name from the given item.
void Dataset::eraseAttr(const std::string &name, const std::string &attrName) {
  expect::contains(*this, name);
  erase_from_map(m_data[name].attrs, attrName);
}

/// Removes an attribute for the given attribute name.
void Dataset::eraseMask(const std::string &maskName) {
  erase_from_map(m_masks, maskName);
}

/// Remove the sparse coordinate with given name.
///
/// Sparse coordinates can exist even without corresponding data.
void Dataset::eraseSparseCoord(const std::string &name) {
  auto iter = m_data.find(name);
  if (iter == m_data.end())
    throw scipp::except::SparseDataError("No sparse data with name " + name +
                                         " found.");

  auto &data = iter->second;
  !data.data ? [this, &iter]() { m_data.erase(iter); }() : data.coord.reset();
  rebuildDims();
}

/// Remove the sparse labels with given name and label name.
void Dataset::eraseSparseLabels(const std::string &name,
                                const std::string &labelName) {
  auto iter = m_data.find(name);
  if (iter == m_data.end())
    throw std::runtime_error("No sparse data with name " + name + " found.");

  auto &data = iter->second;
  auto liter = data.labels.find(labelName);
  if (liter == data.labels.end())
    throw scipp::except::SparseDataError("No sparse data with name " + name +
                                         " found.");

  data.labels.size() == 1 ? data.labels.clear()
                          : [this, &iter]() { m_data.erase(iter); }();
}

/// Return const slice of the dataset along given dimension with given extents.
///
/// This does not make a copy of the data. Instead of view object is returned.
DatasetConstView Dataset::slice(const Slice slice1) const & {
  return DatasetConstView(*this).slice(slice1);
}

/// Return const slice of the dataset, sliced in two dimensions.
///
/// This does not make a copy of the data. Instead of view object is returned.
DatasetConstView Dataset::slice(const Slice slice1,
                                const Slice slice2) const & {
  return DatasetConstView(*this).slice(slice1, slice2);
}

/// Return const slice of the dataset, sliced in three dimensions.
///
/// This does not make a copy of the data. Instead of view object is returned.
DatasetConstView Dataset::slice(const Slice slice1, const Slice slice2,
                                const Slice slice3) const & {
  return DatasetConstView(*this).slice(slice1, slice2, slice3);
}

/// Return slice of the dataset along given dimension with given extents.
///
/// This does not make a copy of the data. Instead of view object is returned.
DatasetView Dataset::slice(const Slice slice1) & {
  return DatasetView(*this).slice(slice1);
}

/// Return slice of the dataset, sliced in two dimensions.
///
/// This does not make a copy of the data. Instead of view object is returned.
DatasetView Dataset::slice(const Slice slice1, const Slice slice2) & {
  return DatasetView(*this).slice(slice1, slice2);
}

/// Return slice of the dataset, sliced in three dimensions.
///
/// This does not make a copy of the data. Instead of view object is returned.
DatasetView Dataset::slice(const Slice slice1, const Slice slice2,
                           const Slice slice3) & {
  return DatasetView(*this).slice(slice1, slice2, slice3);
}

/// Return const slice of the dataset along given dimension with given extents.
///
/// This overload for rvalue reference *this avoids returning a view
/// referencing data that is about to go out of scope and returns a new dataset
/// instead.
Dataset Dataset::slice(const Slice slice1) const && {
  return Dataset{DatasetConstView(*this).slice(slice1)};
}

/// Return const slice of the dataset, sliced in two dimensions.
///
/// This overload for rvalue reference *this avoids returning a view
/// referencing data that is about to go out of scope and returns a new dataset
/// instead.
Dataset Dataset::slice(const Slice slice1, const Slice slice2) const && {
  return Dataset{DatasetConstView(*this).slice(slice1, slice2)};
}

/// Return const slice of the dataset, sliced in three dimensions.
///
/// This overload for rvalue reference *this avoids returning a view
/// referencing data that is about to go out of scope and returns a new dataset
/// instead.
Dataset Dataset::slice(const Slice slice1, const Slice slice2,
                       const Slice slice3) const && {
  return Dataset{DatasetConstView(*this).slice(slice1, slice2, slice3)};
}

/// Rename dimension `from` to `to`.
void Dataset::rename(const Dim from, const Dim to) {
  if (m_dims.count(to) != 0)
    throw except::DimensionError("Duplicate dimension.");
  if (m_dims.count(from) != 1)
    return;

  const auto relabel = [from, to](auto &map) {
    auto node = map.extract(from);
    node.key() = to;
    map.insert(std::move(node));
  };
  relabel(m_dims);
  if (coords().contains(from))
    relabel(m_coords);
  for (auto &item : m_coords)
    item.second.rename(from, to);
  for (auto &item : m_labels)
    item.second.rename(from, to);
  for (auto &item : m_masks)
    item.second.rename(from, to);
  for (auto &item : m_attrs)
    item.second.rename(from, to);
  for (auto &item : m_data) {
    auto &value = item.second;
    if (value.data)
      value.data->rename(from, to);
    if (value.coord)
      value.coord->rename(from, to);
    for (auto &labels : value.labels)
      labels.second.rename(from, to);
    for (auto &attr : value.attrs)
      attr.second.rename(from, to);
  }
}

DataArrayConstView::DataArrayConstView(
    const Dataset &dataset, const detail::dataset_item_map::value_type &data,
    const detail::slice_list &slices, std::optional<VariableView> &&view)
    : m_dataset(&dataset), m_data(&data), m_slices(slices) {
  if (view)
    m_view = std::move(view);
  else if (hasData())
    m_view.emplace(
        VariableView(detail::makeSlice(*m_data->second.data, this->slices())));
}

/// Return the name of the view.
///
/// The name of the view is equal to the name of the item in a Dataset, or the
/// name of a DataArray. Note that comparison operations ignore the name.
const std::string &DataArrayConstView::name() const noexcept {
  return m_data->first;
}

/// Return an ordered mapping of dimension labels to extents, excluding a
/// potentialy sparse dimensions.
Dimensions DataArrayConstView::dims() const noexcept {
  if (hasData())
    return data().dims();
  return detail::makeSlice(*m_data->second.coord, slices()).dims();
}

/// Return the dtype of the data. Throws if there is no data.
DType DataArrayConstView::dtype() const { return data().dtype(); }

/// Return the unit of the data values.
///
/// Throws if there are no data values.
units::Unit DataArrayConstView::unit() const { return data().unit(); }

/// Set the unit of the data values.
///
/// Throws if there are no data values.
void DataArrayView::setUnit(const units::Unit unit) const {
  if (hasData())
    return data().setUnit(unit);
  throw std::runtime_error("Data without values, cannot set unit.");
}

/// Return a const view to all coordinates of the data view.
///
/// If the data has a sparse dimension the returned view will not contain any
/// of the dataset's coordinates that depends on the sparse dimension.
CoordsConstView DataArrayConstView::coords() const noexcept {
  return CoordsConstView(makeViewItems<Dim>(dims(), m_dataset->m_coords,
                                            m_data->second.coord
                                                ? &*m_data->second.coord
                                                : nullptr),
                         slices());
}

/// Return a const view to all labels of the data view.
///
/// If the data has a sparse dimension the returned view will not contain any
/// of the dataset's labels that depends on the sparse dimension.
LabelsConstView DataArrayConstView::labels() const noexcept {
  return LabelsConstView(makeViewItems<std::string>(dims(), m_dataset->m_labels,
                                                    &m_data->second.labels),
                         slices());
}

/// Return a const view to all attributes of the data view.
AttrsConstView DataArrayConstView::attrs() const noexcept {
  return AttrsConstView(
      makeViewItems<std::string>(dims(), m_data->second.attrs), slices());
}

/// Return a const view to all masks of the data view.
MasksConstView DataArrayConstView::masks() const noexcept {
  return MasksConstView(makeViewItems<std::string>(dims(), m_dataset->m_masks),
                        slices());
}

DataArrayConstView DataArrayConstView::slice(const Slice slice1) const {
  const auto &dims_ = dims();
  expect::validSlice(dims_, slice1);
  auto tmp(m_slices);
  tmp.emplace_back(slice1, dims_[slice1.dim()]);
  return {*m_dataset, *m_data, std::move(tmp)};
}

DataArrayConstView DataArrayConstView::slice(const Slice slice1,
                                             const Slice slice2) const {
  return slice(slice1).slice(slice2);
}

DataArrayConstView DataArrayConstView::slice(const Slice slice1,
                                             const Slice slice2,
                                             const Slice slice3) const {
  return slice(slice1, slice2).slice(slice3);
}

DataArrayView::DataArrayView(Dataset &dataset,
                             detail::dataset_item_map::value_type &data,
                             const detail::slice_list &slices)
    : DataArrayConstView(
          dataset, data, slices,
          data.second.data.has_value()
              ? VariableView(detail::makeSlice(*data.second.data, slices))
              : std::optional<VariableView>{}),
      m_mutableDataset(&dataset), m_mutableData(&data) {}

DataArrayView DataArrayView::slice(const Slice slice1) const {
  expect::validSlice(dims(), slice1);
  auto tmp(slices());
  tmp.emplace_back(slice1, dims()[slice1.dim()]);
  return {*m_mutableDataset, *m_mutableData, std::move(tmp)};
}

DataArrayView DataArrayView::slice(const Slice slice1,
                                   const Slice slice2) const {
  return slice(slice1).slice(slice2);
}

DataArrayView DataArrayView::slice(const Slice slice1, const Slice slice2,
                                   const Slice slice3) const {
  return slice(slice1, slice2).slice(slice3);
}

/// Return a view to all coordinates of the data view.
///
/// If the data has a sparse dimension the returned view will not contain any
/// of the dataset's coordinates that depends on the sparse dimension.
CoordsView DataArrayView::coords() const noexcept {
  return CoordsView(m_mutableDataset, &name(),
                    makeViewItems<Dim>(dims(), m_mutableDataset->m_coords,
                                       m_mutableData->second.coord
                                           ? &*m_mutableData->second.coord
                                           : nullptr),
                    slices());
}

/// Return a view to all labels of the data view.
///
/// If the data has a sparse dimension the returned view will not contain any
/// of the dataset's labels that depends on the sparse dimension.
LabelsView DataArrayView::labels() const noexcept {
  return LabelsView(m_mutableDataset, &name(),
                    makeViewItems<std::string>(dims(),
                                               m_mutableDataset->m_labels,
                                               &m_mutableData->second.labels),
                    slices());
}

/// Return a const view to all attributes of the data view.
AttrsView DataArrayView::attrs() const noexcept {
  return AttrsView(
      m_mutableDataset, &name(),
      makeViewItems<std::string>(dims(), m_mutableData->second.attrs),
      slices());
}

/// Return a view to all masks of the data view.
MasksView DataArrayView::masks() const noexcept {
  return MasksView(
      m_mutableDataset, &name(),
      makeViewItems<std::string>(dims(), m_mutableDataset->m_masks), slices());
}

DataArrayView DataArrayView::assign(const DataArrayConstView &other) const {
  if (&underlying() == &other.underlying() && slices() == other.slices())
    return *this; // Self-assignment, return early.

  expect::coordsAndLabelsAreSuperset(*this, other);
  // TODO here and below: If other has data, we should either fail, or create
  // data.
  if (hasData())
    data().assign(other.data());
  return *this;
}

DataArrayView DataArrayView::assign(const Variable &other) const {
  if (hasData())
    data().assign(other);
  return *this;
}

DataArrayView DataArrayView::assign(const VariableConstView &other) const {
  if (hasData())
    data().assign(other);
  return *this;
}

DatasetView DatasetView::assign(const DatasetConstView &other) const {
  for (const auto &data : other)
    operator[](data.name()).assign(data);
  return *this;
}

DatasetConstView::DatasetConstView(const Dataset &dataset)
    : m_dataset(&dataset) {
  m_items.reserve(dataset.size());
  for (const auto &item : dataset.m_data)
    m_items.emplace_back(DataArrayView(detail::make_item{this}(item)));
}

DatasetView::DatasetView(Dataset &dataset)
    : DatasetConstView(DatasetConstView::makeViewWithEmptyIndexes(dataset)),
      m_mutableDataset(&dataset) {
  m_items.reserve(size());
  for (auto &item : dataset.m_data)
    m_items.emplace_back(detail::make_item{this}(item));
}

/// Return a const view to all coordinates of the dataset slice.
///
/// This view includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsConstView DatasetConstView::coords() const noexcept {
  return CoordsConstView(makeViewItems<Dim>(m_dataset->m_coords), slices());
}

/// Return a view to all coordinates of the dataset slice.
///
/// This view includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsView DatasetView::coords() const noexcept {
  auto *parent = slices().empty() ? m_mutableDataset : nullptr;
  return CoordsView(parent, nullptr,
                    makeViewItems<Dim>(m_mutableDataset->m_coords), slices());
}

/// Return a const view to all labels of the dataset slice.
LabelsConstView DatasetConstView::labels() const noexcept {
  return LabelsConstView(makeViewItems<std::string>(m_dataset->m_labels),
                         slices());
}

/// Return a view to all labels of the dataset slice.
LabelsView DatasetView::labels() const noexcept {
  auto *parent = slices().empty() ? m_mutableDataset : nullptr;
  return LabelsView(parent, nullptr,
                    makeViewItems<std::string>(m_mutableDataset->m_labels),
                    slices());
}

/// Return a const view to all attributes of the dataset slice.
AttrsConstView DatasetConstView::attrs() const noexcept {
  return AttrsConstView(makeViewItems<std::string>(m_dataset->m_attrs),
                        slices());
}

/// Return a view to all attributes of the dataset slice.
AttrsView DatasetView::attrs() const noexcept {
  auto *parent = slices().empty() ? m_mutableDataset : nullptr;
  return AttrsView(parent, nullptr,
                   makeViewItems<std::string>(m_mutableDataset->m_attrs),
                   slices());
}
/// Return a const view to all masks of the dataset slice.
MasksConstView DatasetConstView::masks() const noexcept {
  return MasksConstView(makeViewItems<std::string>(m_dataset->m_masks),
                        slices());
}

/// Return a view to all masks of the dataset slice.
MasksView DatasetView::masks() const noexcept {
  auto *parent = slices().empty() ? m_mutableDataset : nullptr;
  return MasksView(parent, nullptr,
                   makeViewItems<std::string>(m_mutableDataset->m_masks),
                   slices());
}

bool DatasetConstView::contains(const std::string &name) const noexcept {
  return find(name) != end();
}

namespace {
template <class T> const auto &getitem(const T &view, const std::string &name) {
  if (auto it = view.find(name); it != view.end())
    return *it;
  throw except::NotFoundError("Expected " + to_string(view) + " to contain " +
                              name + ".");
}
} // namespace

/// Return a const view to data and coordinates with given name.
const DataArrayConstView &DatasetConstView::
operator[](const std::string &name) const {
  return getitem(*this, name);
}

/// Return a view to data and coordinates with given name.
const DataArrayView &DatasetView::operator[](const std::string &name) const {
  return getitem(*this, name);
}

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
    expect::validSlice(currentDims, slice);
    extent = currentDims.at(slice.dim());
  }
  slices.emplace_back(slice, extent);
  return std::pair{std::move(items), std::move(slices)};
}

/// Return a slice of the dataset view.
///
/// The returned view will not contain references to data items that do not
/// depend on the sliced dimension.
DatasetConstView DatasetConstView::slice(const Slice slice1) const {
  DatasetConstView sliced;
  sliced.m_dataset = m_dataset;
  std::tie(sliced.m_items, sliced.m_slices) = slice_items(*this, slice1);
  return sliced;
}

DatasetView DatasetView::slice(const Slice slice1) const {
  DatasetView sliced;
  sliced.m_dataset = m_dataset;
  sliced.m_mutableDataset = m_mutableDataset;
  std::tie(sliced.m_items, sliced.m_slices) = slice_items(*this, slice1);
  return sliced;
}

/// Return true if the dataset proxies have identical content.
bool operator==(const DataArrayConstView &a, const DataArrayConstView &b) {
  if (a.hasData() != b.hasData())
    return false;
  if (a.hasVariances() != b.hasVariances())
    return false;
  if (a.coords() != b.coords())
    return false;
  if (a.labels() != b.labels())
    return false;
  if (a.masks() != b.masks())
    return false;
  if (a.attrs() != b.attrs())
    return false;
  if (a.hasData() && a.data() != b.data())
    return false;
  return true;
}

bool operator!=(const DataArrayConstView &a, const DataArrayConstView &b) {
  return !operator==(a, b);
}

template <class A, class B> bool dataset_equals(const A &a, const B &b) {
  if (a.size() != b.size())
    return false;
  if (a.coords() != b.coords())
    return false;
  if (a.labels() != b.labels())
    return false;
  if (a.masks() != b.masks())
    return false;
  if (a.attrs() != b.attrs())
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

/// Return true if the datasets have identical content.
bool Dataset::operator==(const DatasetConstView &other) const {
  return dataset_equals(*this, other);
}

/// Return true if the datasets have identical content.
bool DatasetConstView::operator==(const Dataset &other) const {
  return dataset_equals(*this, other);
}

/// Return true if the datasets have identical content.
bool DatasetConstView::operator==(const DatasetConstView &other) const {
  return dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content./
bool Dataset::operator!=(const Dataset &other) const {
  return !dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content.
bool Dataset::operator!=(const DatasetConstView &other) const {
  return !dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content.
bool DatasetConstView::operator!=(const Dataset &other) const {
  return !dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content.
bool DatasetConstView::operator!=(const DatasetConstView &other) const {
  return !dataset_equals(*this, other);
}

std::unordered_map<Dim, scipp::index> DatasetConstView::dimensions() const {

  auto base_dims = m_dataset->dimensions();
  // Note current slices are ordered, but NOT unique
  for (const auto &[slice, extents] : m_slices) {
    (void)extents;
    auto it = base_dims.find(slice.dim());
    if (!slice.isRange()) { // For non-range. Erase dimension
      base_dims.erase(it);
    } else {
      it->second = slice.end() -
                   slice.begin(); // Take extent from slice. This is the effect
                                  // that the successful slice range will have
    }
  }
  return base_dims;
}

std::unordered_map<Dim, scipp::index> Dataset::dimensions() const {
  std::unordered_map<Dim, scipp::index> all;
  for (const auto &dim : this->m_dims) {
    all[dim.first] = extents::decodeExtent(dim.second);
  }
  return all;
}

std::map<typename MasksConstView::key_type,
         typename MasksConstView::mapped_type>
union_or(const MasksConstView &currentMasks, const MasksConstView &otherMasks) {
  std::map<typename MasksConstView::key_type,
           typename MasksConstView::mapped_type>
      out;

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

void union_or_in_place(const MasksView &currentMasks,
                       const MasksConstView &otherMasks) {
  for (const auto &[key, item] : otherMasks) {
    const auto it = currentMasks.find(key);
    if (it != currentMasks.end()) {
      it->second |= item;
    } else {
      currentMasks.set(key, item);
    }
  }
}
} // namespace scipp::core
