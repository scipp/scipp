// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <ostream>

#include "dataset.h"
#include "except.h"

namespace scipp::core {

template <class T>
std::pair<const Variable *, Variable *> makeProxyItem(T *variable) {
  if constexpr (std::is_const_v<T>)
    return {variable, nullptr};
  else
    return {variable, variable};
}

template <class Key, class T1> auto makeProxyItems(T1 &coords) {
  std::map<Key, std::pair<const Variable *, Variable *>> items;
  for (auto &item : coords)
    items.emplace(item.first, makeProxyItem(&item.second));
  return items;
}

template <class Key, class T1, class T2 = void>
auto makeProxyItems(const Dimensions &dims, T1 &coords,
                    const Dim sparseDim = Dim::Invalid, T2 *sparse = nullptr) {
  std::map<Key, std::pair<const Variable *, Variable *>> items;
  for (auto &item : coords) {
    // We preserve only items that are part of the space spanned by the
    // provided parent dimensions. Note the use of std::any_of (not
    // std::all_of): At this point there may still be extra dimensions in item,
    // but they will be sliced out. Maybe a better implementation would be to
    // slice the coords first? That would also eliminate a potential loophole
    // for multi-dimensional coordinates.
    auto contained = [&dims](const auto item) {
      const auto &coordDims = item.second.dims();
      if constexpr (std::is_same_v<Key, Dim>)
        return coordDims.empty() || dims.contains(item.first);
      else
        return coordDims.empty() || dims.contains(coordDims.inner());
    };
    if (contained(item)) {
      // Shadow all global coordinates that depend on the sparse dimension.
      if ((sparseDim == Dim::Invalid) ||
          (!item.second.dimensions().contains(sparseDim)))
        items.emplace(item.first, makeProxyItem(&item.second));
    }
  }
  if (sparse) {
    if constexpr (std::is_same_v<T2, const Variable> ||
                  std::is_same_v<T2, Variable>) {
      items.emplace(sparseDim, makeProxyItem(&*sparse));
    } else if constexpr (!std::is_same_v<T2, void>) {
      for (const auto &item : *sparse)
        items.emplace(item.first, makeProxyItem(&item.second));
    }
  }
  return items;
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
  return CoordsProxy(makeProxyItems<Dim>(m_coords));
}

/// Return a const proxy to all labels of the dataset.
LabelsConstProxy Dataset::labels() const noexcept {
  return LabelsConstProxy(makeProxyItems<std::string_view>(m_labels));
}

/// Return a proxy to all labels of the dataset.
LabelsProxy Dataset::labels() noexcept {
  return LabelsProxy(makeProxyItems<std::string_view>(m_labels));
}

/// Return a const proxy to all attributes of the dataset.
AttrsConstProxy Dataset::attrs() const noexcept {
  return AttrsConstProxy(makeProxyItems<std::string_view>(m_attrs));
}

/// Return a proxy to all attributes of the dataset.
AttrsProxy Dataset::attrs() noexcept {
  return AttrsProxy(makeProxyItems<std::string_view>(m_attrs));
}

/// Return a const proxy to data and coordinates with given name.
DataConstProxy Dataset::operator[](const std::string_view name) const {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::out_of_range("Could not find data with name " +
                            std::string(name) + ".");
  return DataConstProxy(*this, it->second);
}

/// Return a proxy to data and coordinates with given name.
DataProxy Dataset::operator[](const std::string_view name) {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::out_of_range("Could not find data with name " +
                            std::string(name) + ".");
  return DataProxy(*this, it->second);
}

/// Helper for setDims().
void Dataset::setExtent(const Dim dim, const scipp::index extent,
                        const bool isCoord) {
  const auto it = m_dims.find(dim);
  // Internally use negative extent -1 to indicate unknown edge state. The `-1`
  // is required for dimensions with extent 0.
  if (it == m_dims.end()) {
    m_dims[dim] = -extent - 1;
  } else {
    if (it->second < 0) {
      if (extent == -it->second - 1) {
      } else if (extent == (-it->second - 1 + 1) && isCoord) {
        it->second = extent - 1;
      } else if (extent == (-it->second - 1 - 1) && !isCoord) {
        it->second = extent;
      } else {
        throw std::runtime_error("Length mismatch on insertion");
      }
    } else {
      if ((extent != it->second || isCoord) && extent != it->second + 1)
        throw std::runtime_error("Length mismatch on insertion");
    }
  }
}

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
  for (const auto dim : dims.labels())
    setExtent(dim, dims[dim], dim == coordDim);
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

template <class A, class B>
void check_dtype(const A &values, const B &variances) {
  if (values.dtype() != variances.dtype())
    throw std::runtime_error("Values and variances must have the same dtype.");
}

template <class A, class B>
void check_unit(const A &values, const B &variances) {
  const auto unit = values.unit();
  if (variances.unit() != unit * unit)
    throw std::runtime_error(
        "Values and variances must have compatible units.");
}

template <class A, class B>
void check_dimensions(const A &values, const B &variances) {
  if ((values.dimensions() != variances.dimensions()) ||
      values.sparseDim() != variances.sparseDim())
    throw std::runtime_error(
        "Values and variances must have identical dimensions.");
}

/// Set (insert or replace) the data values with given name.
///
/// Throws if the provided values bring the dataset into an inconsistent state
/// (mismatching dtype, unit, or dimensions).
void Dataset::setValues(const std::string &name, Variable values) {
  setDims(values.dims());
  const auto it = m_data.find(name);
  if (it != m_data.end() && it->second.variances) {
    const auto &variances = *it->second.variances;
    check_dtype(values, variances);
    check_unit(values, variances);
    check_dimensions(values, variances);
  }
  m_data[name].values = std::move(values);
}

/// Set (insert or replace) the data variances with given name.
///
/// Throws if the provided variances bring the dataset into an inconsistent
/// state (mismatching dtype, unit, or dimensions).
void Dataset::setVariances(const std::string &name, Variable variances) {
  setDims(variances.dims());
  const auto it = m_data.find(name);
  if (it == m_data.end() || !it->second.values)
    throw std::runtime_error("Cannot set variances: No data values for " +
                             name + " found in dataset.");
  const auto &values = *it->second.values;
  check_dtype(values, variances);
  check_unit(values, variances);
  check_dimensions(values, variances);
  m_data.at(name).variances = std::move(variances);
}

/// Set (insert or replace) the sparse coordinate with given name.
///
/// Sparse coordinates can exist even without corresponding data.
void Dataset::setSparseCoord(const std::string &name, Variable coord) {
  setDims(coord.dims());
  if (!coord.isSparse())
    throw std::runtime_error("Variable passed to Dataset::setSparseCoord does "
                             "not contain sparse data.");
  if (m_data.count(name)) {
    const auto &data = m_data.at(name);
    if ((data.values && (data.values->sparseDim() != coord.sparseDim())) ||
        (!data.labels.empty() &&
         (data.labels.begin()->second.sparseDim() != coord.sparseDim())))
      throw std::runtime_error("Cannot set sparse coordinate if values or "
                               "variances are not sparse.");
  }
  m_data[name].coord = std::move(coord);
}

/// Set (insert or replace) the sparse labels with given name and label name.
void Dataset::setSparseLabels(const std::string &name,
                              const std::string &labelName, Variable labels) {
  setDims(labels.dims());
  if (!labels.isSparse())
    throw std::runtime_error("Variable passed to Dataset::setSparseLabels does "
                             "not contain sparse data.");
  if (m_data.count(name)) {
    const auto &data = m_data.at(name);
    if ((data.values && (data.values->sparseDim() != labels.sparseDim())) ||
        (data.coord && (data.coord->sparseDim() != labels.sparseDim())))
      throw std::runtime_error("Cannot set sparse labels if values or "
                               "variances are not sparse.");
  }
  const auto &data = m_data.at(name);
  if (!data.values && !data.coord)
    throw std::runtime_error(
        "Cannot set sparse labels: Require either values or a sparse coord.");

  m_data[name].labels.insert_or_assign(labelName, std::move(labels));
}

/// Return const slice of the dataset along given dimension with given extents.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetConstProxy Dataset::slice(const Slice slice1) const {
  return DatasetConstProxy(*this).slice(slice1);
}

/// Return const slice of the dataset, sliced in two dimensions.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetConstProxy Dataset::slice(const Slice slice1, const Slice slice2) const {
  return DatasetConstProxy(*this).slice(slice1, slice2);
}

/// Return const slice of the dataset, sliced in three dimensions.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetConstProxy Dataset::slice(const Slice slice1, const Slice slice2,
                                 const Slice slice3) const {
  return DatasetConstProxy(*this).slice(slice1, slice2, slice3);
}

/// Return slice of the dataset along given dimension with given extents.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetProxy Dataset::slice(const Slice slice1) {
  return DatasetProxy(*this).slice(slice1);
}

/// Return slice of the dataset, sliced in two dimensions.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetProxy Dataset::slice(const Slice slice1, const Slice slice2) {
  return DatasetProxy(*this).slice(slice1, slice2);
}

/// Return slice of the dataset, sliced in three dimensions.
///
/// This does not make a copy of the data. Instead of proxy object is returned.
DatasetProxy Dataset::slice(const Slice slice1, const Slice slice2,
                            const Slice slice3) {
  return DatasetProxy(*this).slice(slice1, slice2, slice3);
}

/// Return true if the proxy represents sparse data.
bool DataConstProxy::isSparse() const noexcept {
  if (m_data->coord)
    return true;
  if (hasValues())
    return values().isSparse();
  return false;
}

/// Return the label of the sparse dimension, Dim::Invalid if there is none.
Dim DataConstProxy::sparseDim() const noexcept {
  if (m_data->coord)
    return m_data->coord->sparseDim();
  if (hasValues())
    return values().sparseDim();
  return Dim::Invalid;
}

/// Return an ordered mapping of dimension labels to extents, excluding a
/// potentialy sparse dimensions.
Dimensions DataConstProxy::dims() const noexcept {
  if (hasValues())
    return values().dimensions();
  return detail::makeSlice(*m_data->coord, slices()).dimensions();
}

/// Return the unit of the data values.
///
/// Throws if there are no data values.
units::Unit DataConstProxy::unit() const {
  if (hasValues())
    return values().unit();
  throw std::runtime_error("Data without values, unit is undefined.");
}

/// Return a const proxy to all coordinates of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's coordinates that depends on the sparse dimension.
CoordsConstProxy DataConstProxy::coords() const noexcept {
  return CoordsConstProxy(
      makeProxyItems<Dim>(dims(), m_dataset->m_coords, sparseDim(),
                          m_data->coord ? &*m_data->coord : nullptr),
      slices());
}

/// Return a const proxy to all labels of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's labels that depends on the sparse dimension.
LabelsConstProxy DataConstProxy::labels() const noexcept {
  return LabelsConstProxy(
      makeProxyItems<std::string_view>(dims(), m_dataset->m_labels, sparseDim(),
                                       &m_data->labels),
      slices());
}

/// Return a const proxy to all attributes of the data proxy.
AttrsConstProxy DataConstProxy::attrs() const noexcept {
  return AttrsConstProxy(
      makeProxyItems<std::string_view>(dims(), m_dataset->m_attrs), slices());
}

/// Return a proxy to all coordinates of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's coordinates that depends on the sparse dimension.
CoordsProxy DataProxy::coords() const noexcept {
  return CoordsProxy(
      makeProxyItems<Dim>(dims(), m_mutableDataset->m_coords, sparseDim(),
                          m_mutableData->coord ? &*m_mutableData->coord
                                               : nullptr),
      slices());
}

/// Return a proxy to all labels of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's labels that depends on the sparse dimension.
LabelsProxy DataProxy::labels() const noexcept {
  return LabelsProxy(
      makeProxyItems<std::string_view>(dims(), m_mutableDataset->m_labels,
                                       sparseDim(), &m_mutableData->labels),
      slices());
}

/// Return a const proxy to all attributes of the data proxy.
AttrsProxy DataProxy::attrs() const noexcept {
  return AttrsProxy(
      makeProxyItems<std::string_view>(dims(), m_mutableDataset->m_attrs),
      slices());
}

DataProxy DataProxy::operator+=(const DataConstProxy &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  if (hasValues())
    values() += other.values();
  if (other.hasVariances()) {
    if (hasVariances())
      variances() += other.variances();
    else
      throw std::runtime_error(
          "RHS in operation has variances but LHS does not.");
  }

  return *this;
}

DataProxy DataProxy::operator*=(const DataConstProxy &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  if (hasVariances()) {
    if (other.hasVariances())
      variances().assign(variances() * (other.values() * other.values()) +
                         (values() * values()) * other.variances());
    else
      variances().assign(variances() * (other.values() * other.values()));
  } else if (other.hasVariances()) {
    throw std::runtime_error(
        "RHS in operation has variances but LHS does not.");
  }
  if (hasValues())
    values() *= other.values();
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
  return CoordsProxy(makeProxyItems<Dim>(m_mutableDataset->m_coords), slices());
}

/// Return a const proxy to all labels of the dataset slice.
LabelsConstProxy DatasetConstProxy::labels() const noexcept {
  return LabelsConstProxy(makeProxyItems<std::string_view>(m_dataset->m_labels),
                          slices());
}

/// Return a proxy to all labels of the dataset slice.
LabelsProxy DatasetProxy::labels() const noexcept {
  return LabelsProxy(
      makeProxyItems<std::string_view>(m_mutableDataset->m_labels), slices());
}

/// Return a const proxy to all attributes of the dataset slice.
AttrsConstProxy DatasetConstProxy::attrs() const noexcept {
  return AttrsConstProxy(makeProxyItems<std::string_view>(m_dataset->m_attrs),
                         slices());
}

/// Return a proxy to all attributes of the dataset slice.
AttrsProxy DatasetProxy::attrs() const noexcept {
  return AttrsProxy(makeProxyItems<std::string_view>(m_mutableDataset->m_attrs),
                    slices());
}

void DatasetConstProxy::expectValidKey(const std::string_view name) const {
  if (std::find(m_indices.begin(), m_indices.end(), name) == m_indices.end())
    throw std::out_of_range("Invalid key `" + std::string(name) +
                            "` in Dataset access.");
}

/// Return a const proxy to data and coordinates with given name.
DataConstProxy DatasetConstProxy::
operator[](const std::string_view name) const {
  expectValidKey(name);
  return {*m_dataset, (*m_dataset).m_data.find(name)->second, slices()};
}

/// Return a proxy to data and coordinates with given name.
DataProxy DatasetProxy::operator[](const std::string_view name) const {
  expectValidKey(name);
  return {*m_mutableDataset, (*m_mutableDataset).m_data.find(name)->second,
          slices()};
}

/// Return true if the dataset proxies have identical content.
bool DataConstProxy::operator==(const DataConstProxy &other) const {
  if (hasValues() != other.hasValues())
    return false;
  if (hasVariances() != other.hasVariances())
    return false;
  if (coords() != other.coords())
    return false;
  if (labels() != other.labels())
    return false;
  if (attrs() != other.attrs())
    return false;
  if (hasValues() && values() != other.values())
    return false;
  if (hasVariances() && variances() != other.variances())
    return false;
  return true;
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
  for (const auto & [ name, data ] : a) {
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

/// Return true if the datasets have mismatching content.
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

Dataset &Dataset::operator+=(const DataConstProxy &other) {
  // For `other` referencing data in *this we delay operation. The alternative
  // would be to make a deep copy of `other` before starting the iteration over
  // items.
  std::optional<std::string_view> delayed;
  // Note the inefficiency here: We are comparing some or all of the coords and
  // labels for each item. This could be improved by implementing the operations
  // for detail::DatasetData instead of DataProxy.
  for (const auto item : *this) {
    if (&item.second.data() == &other.data())
      delayed = item.first;
    else
      item.second += other;
  }
  if (delayed)
    operator[](*delayed) += other;
  return *this;
}
Dataset &Dataset::operator*=(const DataConstProxy &other) {
  std::optional<std::string_view> delayed;
  for (const auto item : *this) {
    if (&item.second.data() == &other.data())
      delayed = item.first;
    else
      item.second *= other;
  }
  if (delayed)
    operator[](*delayed) *= other;
  return *this;
}

Dataset &Dataset::operator+=(const DatasetConstProxy &other) {
  for (const auto & [ name, item ] : other)
    operator[](name) += item;
  return *this;
}
Dataset &Dataset::operator*=(const DatasetConstProxy &other) {
  for (const auto & [ name, item ] : other)
    operator[](name) *= item;
  return *this;
}
Dataset &Dataset::operator+=(const Dataset &other) {
  for (const auto & [ name, item ] : other)
    operator[](name) += item;
  return *this;
}
Dataset &Dataset::operator*=(const Dataset &other) {
  for (const auto & [ name, item ] : other)
    operator[](name) *= item;
  return *this;
}

std::ostream &operator<<(std::ostream &os, const DataConstProxy &data) {
  // TODO sparse
  if (data.hasValues())
    os << data.values();
  if (data.hasVariances())
    os << data.variances();
  return os;
}

std::ostream &operator<<(std::ostream &os, const DataProxy &data) {
  return os << DataConstProxy(data);
}

std::ostream &operator<<(std::ostream &os, const DatasetConstProxy &dataset) {
  os << "Coordinates:\n";
  for (const auto & [ name, coord ] : dataset.coords())
    os << to_string(name) << " " << coord;
  os << "Labels:\n";
  for (const auto & [ name, labels ] : dataset.labels())
    os << name << " " << labels;
  os << "Attributes:\n";
  for (const auto & [ name, attr ] : dataset.attrs())
    os << name << " " << attr;
  os << "Data:\n";
  for (const auto & [ name, data ] : dataset)
    os << name << " " << data;
  return os;
}

std::ostream &operator<<(std::ostream &os, const DatasetProxy &dataset) {
  return os << DatasetConstProxy(dataset);
}

std::ostream &operator<<(std::ostream &os, const Dataset &dataset) {
  return os << DatasetConstProxy(dataset);
}

std::ostream &operator<<(std::ostream &os, const ConstVariableSlice &variable) {
  return os << to_string(variable) << " "
            << array_to_string(variable.values<double>()) << std::endl;
}

std::ostream &operator<<(std::ostream &os, const VariableSlice &variable) {
  return os << ConstVariableSlice(variable);
}

std::ostream &operator<<(std::ostream &os, const Variable &variable) {
  return os << ConstVariableSlice(variable);
}

std::ostream &operator<<(std::ostream &os, const Dim dim) {
  return os << to_string(dim);
}

} // namespace scipp::core
