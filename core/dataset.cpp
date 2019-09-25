// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"

namespace scipp::core {

template <class T>
std::pair<const Variable *, Variable *> makeProxyItem(T *variable) {
  if constexpr (std::is_const_v<T>)
    return {variable, nullptr};
  else
    return {variable, variable};
}

template <class Key, class T1> auto makeProxyItems(T1 &coords) {
  std::unordered_map<Key, std::pair<const Variable *, Variable *>> items;
  for (auto &item : coords)
    items.emplace(item.first, makeProxyItem(&item.second));
  return items;
}

template <class Key, class T1, class T2 = void>
auto makeProxyItems(const Dimensions &dims, T1 &coords, T2 *sparse = nullptr) {
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
      if constexpr (std::is_same_v<Key, Dim>)
        return coordDims.empty() || dims.contains(item2.first);
      else
        return coordDims.empty() || dims.contains(coordDims.inner());
    };
    if (contained(item)) {
      // Shadow all global coordinates that depend on the sparse dimension.
      if ((!dims.sparse()) || (!item.second.dims().contains(sparseDim)))
        items.emplace(item.first, makeProxyItem(&item.second));
    }
  }
  if (sparse) {
    if constexpr (std::is_same_v<T2, const Variable> ||
                  std::is_same_v<T2, Variable>) {
      items.emplace(sparseDim, makeProxyItem(sparse));
    } else if constexpr (!std::is_same_v<T2, void>) {
      for (auto &item : *sparse)
        items.emplace(item.first, makeProxyItem(&item.second));
    }
  }
  return items;
}

Dataset::Dataset(const DatasetConstProxy &proxy)
    : Dataset(proxy, proxy.coords(), proxy.labels(), proxy.attrs()) {}

Dataset::Dataset(const DataConstProxy &data) { setData(data.name(), data); }

Dataset::Dataset(const std::map<std::string, DataConstProxy> &data) {
  for (const auto &[name, item] : data)
    setData(name, item);
}

/// Removes all data items from the Dataset.
///
/// Coordinates, labels, attributes and masks are not modified.
/// This operation invalidates any proxy objects creeated from this dataset.
void Dataset::clear() {
  m_data.clear();
  rebuildDims();
}

/// Return a const proxy to all coordinates of the dataset.
///
/// This proxy includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsConstProxy Dataset::coords() const noexcept {
  return CoordsConstProxy(makeProxyItems<Dim>(m_coords));
}

/// Return a proxy to all coordinates of the dataset.
///
/// This proxy includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsProxy Dataset::coords() noexcept {
  return CoordsProxy(this, nullptr, makeProxyItems<Dim>(m_coords));
}

/// Return a const proxy to all labels of the dataset.
LabelsConstProxy Dataset::labels() const noexcept {
  return LabelsConstProxy(makeProxyItems<std::string>(m_labels));
}

/// Return a proxy to all labels of the dataset.
LabelsProxy Dataset::labels() noexcept {
  return LabelsProxy(this, nullptr, makeProxyItems<std::string>(m_labels));
}

/// Return a const proxy to all attributes of the dataset.
AttrsConstProxy Dataset::attrs() const noexcept {
  return AttrsConstProxy(makeProxyItems<std::string>(m_attrs));
}

/// Return a proxy to all attributes of the dataset.
AttrsProxy Dataset::attrs() noexcept {
  return AttrsProxy(this, nullptr, makeProxyItems<std::string>(m_attrs));
}

/// Return a const proxy to all masks of the dataset.
MasksConstProxy Dataset::masks() const noexcept {
  return MasksConstProxy(makeProxyItems<std::string>(m_masks));
}

/// Return a proxy to all masks of the dataset.
MasksProxy Dataset::masks() noexcept {
  return MasksProxy(this, nullptr, makeProxyItems<std::string>(m_masks));
}

bool Dataset::contains(const std::string &name) const noexcept {
  return m_data.count(name) == 1;
}

/// Removes a data item from the Dataset
///
/// Coordinates, labels and attributes are not modified.
/// This operation invalidates any proxy objects creeated from this dataset.
void Dataset::erase(const std::string_view name) {
  if (m_data.erase(std::string(name)) == 0) {
    throw except::DatasetError(*this, "Could not find data with name " +
                                          std::string(name) + ".");
  }
  rebuildDims();
}

/// Return a const proxy to data and coordinates with given name.
DataConstProxy Dataset::operator[](const std::string &name) const {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::out_of_range("Could not find data with name " +
                            std::string(name) + ".");
  return DataConstProxy(*this, *it);
}

/// Return a proxy to data and coordinates with given name.
DataProxy Dataset::operator[](const std::string &name) {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::out_of_range("Could not find data with name " +
                            std::string(name) + ".");
  return DataProxy(*this, *it);
}

namespace extents {
// Internally use negative extent -1 to indicate unknown edge state. The `-1`
// is required for dimensions with extent 0.
scipp::index makeUnknownEdgeState(const scipp::index extent) {
  return -extent - 1;
}
scipp::index shrink(const scipp::index extent) { return extent - 1; }
bool isUnknownEdgeState(const scipp::index extent) { return extent < 0; }
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
  } else {
    auto &heldExtent = it->second;
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
  m_dims = tmp;
}

void Dataset::rebuildDims() {
  /* Clear old extents */
  m_dims.clear();

  for (const auto &d : *this) {
    setDims(d.second.dims());
  }
  for (const auto &c : m_coords) {
    setDims(c.second.dims(), c.first);
  }
  for (const auto &l : m_labels) {
    setDims(l.second.dims());
  }
  for (const auto &a : m_attrs) {
    setDims(a.second.dims());
  }
}

/// Set (insert or replace) the coordinate for the given dimension.
void Dataset::setCoord(const Dim dim, Variable coord) {
  setDims(coord.dims(), dim);
  m_coords.insert_or_assign(dim, std::move(coord));
}

/// Set (insert or replace) the labels for the given label name.
///
/// Note that the label name has no relation to names of data items.
void Dataset::setLabels(const std::string &labelName, Variable labels) {
  setDims(labels.dims());
  m_labels.insert_or_assign(labelName, std::move(labels));
}

/// Set (insert or replace) an attribute for the given attribute name.
///
/// Note that the attribute name has no relation to names of data items.
void Dataset::setAttr(const std::string &attrName, Variable attr) {
  setDims(attr.dims());
  m_attrs.insert_or_assign(attrName, std::move(attr));
}

/// Set (insert or replace) the masks for the given label name.
///
/// Note that the label name has no relation to names of data items.
void Dataset::setMasks(const std::string &labelName, Variable masks) {
  setDims(masks.dims());
  m_masks.insert_or_assign(labelName, std::move(masks));
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
  m_data[name].data = std::move(data);
}

/// Set (insert or replace) data item with given name.
///
/// Coordinates, labels, and attributes of the data array are added to the
/// dataset. Throws if there are existing but mismatching coords, labels, or
/// attributes. Throws if the provided data brings the dataset into an
/// inconsistent state (mismatching dtype, unit, or dimensions).
void Dataset::setData(const std::string &name, const DataConstProxy &data) {
  for (const auto &[dim, coord] : data.coords()) {
    if (coord.dims().sparse()) {
      setSparseCoord(name, coord);
    } else {
      if (const auto it = m_coords.find(dim); it != m_coords.end())
        expect::equals(coord, it->second);
      else
        setCoord(dim, coord);
    }
  }
  for (const auto &[nm, labs] : data.labels()) {
    if (labs.dims().sparse()) {
      setSparseLabels(name, std::string(nm), labs);
    } else {
      if (const auto it = m_labels.find(std::string(nm)); it != m_labels.end())
        expect::equals(labs, it->second);
      else
        setLabels(std::string(nm), labs);
    }
  }
  for (const auto &[nm, attr] : data.attrs()) {
    if (const auto it = m_attrs.find(std::string(nm)); it != m_attrs.end())
      expect::equals(attr, it->second);
    else
      setAttr(std::string(nm), attr);
  }

  for (const auto &[nm, mask] : data.masks()) {
    setMasks(std::string(nm), mask);
  }

  if (data.hasData())
    setData(name, data.data());
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
  setDims(labels.dims());
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

  m_data[name].labels.insert_or_assign(labelName, std::move(labels));
}

/// Return const slice of the dataset along given dimension with given extents.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetConstProxy Dataset::slice(const Slice slice1) const & {
  return DatasetConstProxy(*this).slice(slice1);
}

/// Return const slice of the dataset, sliced in two dimensions.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetConstProxy Dataset::slice(const Slice slice1,
                                 const Slice slice2) const & {
  return DatasetConstProxy(*this).slice(slice1, slice2);
}

/// Return const slice of the dataset, sliced in three dimensions.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetConstProxy Dataset::slice(const Slice slice1, const Slice slice2,
                                 const Slice slice3) const & {
  return DatasetConstProxy(*this).slice(slice1, slice2, slice3);
}

/// Return slice of the dataset along given dimension with given extents.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetProxy Dataset::slice(const Slice slice1) & {
  return DatasetProxy(*this).slice(slice1);
}

/// Return slice of the dataset, sliced in two dimensions.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetProxy Dataset::slice(const Slice slice1, const Slice slice2) & {
  return DatasetProxy(*this).slice(slice1, slice2);
}

/// Return slice of the dataset, sliced in three dimensions.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetProxy Dataset::slice(const Slice slice1, const Slice slice2,
                            const Slice slice3) & {
  return DatasetProxy(*this).slice(slice1, slice2, slice3);
}

/// Return const slice of the dataset along given dimension with given extents.
///
/// This overload for rvalue reference *this avoids returning a proxy
/// referencing data that is about to go out of scope and returns a new dataset
/// instead.
Dataset Dataset::slice(const Slice slice1) const && {
  return Dataset{DatasetConstProxy(*this).slice(slice1)};
}

/// Return const slice of the dataset, sliced in two dimensions.
///
/// This overload for rvalue reference *this avoids returning a proxy
/// referencing data that is about to go out of scope and returns a new dataset
/// instead.
Dataset Dataset::slice(const Slice slice1, const Slice slice2) const && {
  return Dataset{DatasetConstProxy(*this).slice(slice1, slice2)};
}

/// Return const slice of the dataset, sliced in three dimensions.
///
/// This overload for rvalue reference *this avoids returning a proxy
/// referencing data that is about to go out of scope and returns a new dataset
/// instead.
Dataset Dataset::slice(const Slice slice1, const Slice slice2,
                       const Slice slice3) const && {
  return Dataset{DatasetConstProxy(*this).slice(slice1, slice2, slice3)};
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
  relabel(m_coords);
  for (auto &item : m_coords)
    item.second.rename(from, to);
  for (auto &item : m_labels)
    item.second.rename(from, to);
  for (auto &item : m_attrs)
    item.second.rename(from, to);
  for (auto &item : m_data) {
    auto &value = item.second;
    if (value.data)
      value.data->rename(from, to);
    if (value.coord)
      value.coord->rename(from, to);
    for (auto labels : value.labels)
      labels.second.rename(from, to);
  }
}

/// Return the name of the proxy.
///
/// The name of the proxy is equal to the name of the item in a Dataset, or the
/// name of a DataArray. Note that comparison operations ignore the name.
const std::string &DataConstProxy::name() const noexcept {
  return m_data->first;
}

/// Return an ordered mapping of dimension labels to extents, excluding a
/// potentialy sparse dimensions.
Dimensions DataConstProxy::dims() const noexcept {
  if (hasData())
    return data().dims();
  return detail::makeSlice(*m_data->second.coord, slices()).dims();
}

/// Return the dtype of the data. Throws if there is no data.
DType DataConstProxy::dtype() const { return data().dtype(); }

/// Return the unit of the data values.
///
/// Throws if there are no data values.
units::Unit DataConstProxy::unit() const { return data().unit(); }

/// Set the unit of the data values.
///
/// Throws if there are no data values.
void DataProxy::setUnit(const units::Unit unit) const {
  if (hasData())
    return data().setUnit(unit);
  throw std::runtime_error("Data without values, cannot set unit.");
}

/// Return a const proxy to all coordinates of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's coordinates that depends on the sparse dimension.
CoordsConstProxy DataConstProxy::coords() const noexcept {
  return CoordsConstProxy(makeProxyItems<Dim>(dims(), m_dataset->m_coords,
                                              m_data->second.coord
                                                  ? &*m_data->second.coord
                                                  : nullptr),
                          slices());
}

/// Return a const proxy to all labels of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's labels that depends on the sparse dimension.
LabelsConstProxy DataConstProxy::labels() const noexcept {
  return LabelsConstProxy(makeProxyItems<std::string>(dims(),
                                                      m_dataset->m_labels,
                                                      &m_data->second.labels),
                          slices());
}

/// Return a const proxy to all attributes of the data proxy.
AttrsConstProxy DataConstProxy::attrs() const noexcept {
  return AttrsConstProxy(
      makeProxyItems<std::string>(dims(), m_dataset->m_attrs), slices());
}

/// Return a const proxy to all masks of the data proxy.
MasksConstProxy DataConstProxy::masks() const noexcept {
  return MasksConstProxy(
      makeProxyItems<std::string>(dims(), m_dataset->m_masks), slices());
}

/// Return a proxy to all coordinates of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's coordinates that depends on the sparse dimension.
CoordsProxy DataProxy::coords() const noexcept {
  return CoordsProxy(m_mutableDataset, &name(),
                     makeProxyItems<Dim>(dims(), m_mutableDataset->m_coords,
                                         m_mutableData->second.coord
                                             ? &*m_mutableData->second.coord
                                             : nullptr),
                     slices());
}

/// Return a proxy to all labels of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's labels that depends on the sparse dimension.
LabelsProxy DataProxy::labels() const noexcept {
  return LabelsProxy(m_mutableDataset, &name(),
                     makeProxyItems<std::string>(dims(),
                                                 m_mutableDataset->m_labels,
                                                 &m_mutableData->second.labels),
                     slices());
}

/// Return a const proxy to all attributes of the data proxy.
AttrsProxy DataProxy::attrs() const noexcept {
  return AttrsProxy(
      m_mutableDataset, &name(),
      makeProxyItems<std::string>(dims(), m_mutableDataset->m_attrs), slices());
}

/// Return a proxy to all masks of the data proxy.
MasksProxy DataProxy::masks() const noexcept {
  return MasksProxy(
      m_mutableDataset, &name(),
      makeProxyItems<std::string>(dims(), m_mutableDataset->m_masks), slices());
}

DataProxy DataProxy::assign(const DataConstProxy &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  // TODO here and below: If other has data, we should either fail, or create
  // data.
  if (hasData())
    data().assign(other.data());
  return *this;
}

DataProxy DataProxy::assign(const Variable &other) const {
  if (hasData())
    data().assign(other);
  return *this;
}

DataProxy DataProxy::assign(const VariableConstProxy &other) const {
  if (hasData())
    data().assign(other);
  return *this;
}

/// Return a const proxy to all coordinates of the dataset slice.
///
/// This proxy includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsConstProxy DatasetConstProxy::coords() const noexcept {
  return CoordsConstProxy(makeProxyItems<Dim>(m_dataset->m_coords), slices());
}

/// Return a proxy to all coordinates of the dataset slice.
///
/// This proxy includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsProxy DatasetProxy::coords() const noexcept {
  auto *parent = slices().empty() ? m_mutableDataset : nullptr;
  return CoordsProxy(parent, nullptr,
                     makeProxyItems<Dim>(m_mutableDataset->m_coords), slices());
}

/// Return a const proxy to all labels of the dataset slice.
LabelsConstProxy DatasetConstProxy::labels() const noexcept {
  return LabelsConstProxy(makeProxyItems<std::string>(m_dataset->m_labels),
                          slices());
}

/// Return a proxy to all labels of the dataset slice.
LabelsProxy DatasetProxy::labels() const noexcept {
  auto *parent = slices().empty() ? m_mutableDataset : nullptr;
  return LabelsProxy(parent, nullptr,
                     makeProxyItems<std::string>(m_mutableDataset->m_labels),
                     slices());
}

/// Return a const proxy to all attributes of the dataset slice.
AttrsConstProxy DatasetConstProxy::attrs() const noexcept {
  return AttrsConstProxy(makeProxyItems<std::string>(m_dataset->m_attrs),
                         slices());
}

/// Return a proxy to all attributes of the dataset slice.
AttrsProxy DatasetProxy::attrs() const noexcept {
  auto *parent = slices().empty() ? m_mutableDataset : nullptr;
  return AttrsProxy(parent, nullptr,
                    makeProxyItems<std::string>(m_mutableDataset->m_attrs),
                    slices());
}
/// Return a const proxy to all masks of the dataset slice.
MasksConstProxy DatasetConstProxy::masks() const noexcept {
  return MasksConstProxy(makeProxyItems<std::string>(m_dataset->m_masks),
                         slices());
}

/// Return a proxy to all masks of the dataset slice.
MasksProxy DatasetProxy::masks() const noexcept {
  auto *parent = slices().empty() ? m_mutableDataset : nullptr;
  return MasksProxy(parent, nullptr,
                    makeProxyItems<std::string>(m_mutableDataset->m_masks),
                    slices());
}
void DatasetConstProxy::expectValidKey(const std::string &name) const {
  if (std::find(m_indices.begin(), m_indices.end(), name) == m_indices.end())
    throw std::out_of_range("Invalid key `" + std::string(name) +
                            "` in Dataset access.");
}

bool DatasetConstProxy::contains(const std::string &name) const noexcept {
  return std::find(m_indices.begin(), m_indices.end(), name) != m_indices.end();
}

/// Return a const proxy to data and coordinates with given name.
DataConstProxy DatasetConstProxy::operator[](const std::string &name) const {
  expectValidKey(name);
  return {*m_dataset, *(*m_dataset).m_data.find(name), slices()};
}

/// Return a proxy to data and coordinates with given name.
DataProxy DatasetProxy::operator[](const std::string &name) const {
  expectValidKey(name);
  return {*m_mutableDataset, *(*m_mutableDataset).m_data.find(name), slices()};
}

/// Return true if the dataset proxies have identical content.
bool operator==(const DataConstProxy &a, const DataConstProxy &b) {
  if (a.hasData() != b.hasData())
    return false;
  if (a.hasVariances() != b.hasVariances())
    return false;
  if (a.coords() != b.coords())
    return false;
  if (a.labels() != b.labels())
    return false;
  if (a.attrs() != b.attrs())
    return false;
  if (a.masks() != b.masks())
    return false;
  if (a.hasData() && a.data() != b.data())
    return false;
  return true;
}

bool operator!=(const DataConstProxy &a, const DataConstProxy &b) {
  return !operator==(a, b);
}

template <class A, class B> bool dataset_equals(const A &a, const B &b) {
  if (a.size() != b.size())
    return false;
  if (a.coords() != b.coords())
    return false;
  if (a.labels() != b.labels())
    return false;
  if (a.attrs() != b.attrs())
    return false;
  if (a.masks() != b.masks())
    return false;
  for (const auto &[name, data] : a) {
    try {
      if (data != b[std::string(name)])
        return false;
    } catch (std::out_of_range &) {
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
bool Dataset::operator==(const DatasetConstProxy &other) const {
  return dataset_equals(*this, other);
}

/// Return true if the datasets have identical content.
bool DatasetConstProxy::operator==(const Dataset &other) const {
  return dataset_equals(*this, other);
}

/// Return true if the datasets have identical content.
bool DatasetConstProxy::operator==(const DatasetConstProxy &other) const {
  return dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content./
bool Dataset::operator!=(const Dataset &other) const {
  return !dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content.
bool Dataset::operator!=(const DatasetConstProxy &other) const {
  return !dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content.
bool DatasetConstProxy::operator!=(const Dataset &other) const {
  return !dataset_equals(*this, other);
}

/// Return true if the datasets have mismatching content.
bool DatasetConstProxy::operator!=(const DatasetConstProxy &other) const {
  return !dataset_equals(*this, other);
}

std::unordered_map<Dim, scipp::index> DatasetConstProxy::dimensions() const {

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

} // namespace scipp::core
