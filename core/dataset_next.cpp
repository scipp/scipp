// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dataset_next.h"
#include "dataset.h"
#include "except.h"

namespace scipp::core::next {

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
  if (sparseDim == Dim::Invalid) {
    for (auto &item : coords) {
      // We preserve only items that are part of the space spanned by the
      // provided parent dimensions. Note that Dimensions::contains(const
      // Dimensions &) is not doing the job here, since the extents may differ
      // before slicing.
      const auto &labels = item.second.dimensions().labels();
      if (std::all_of(labels.begin(), labels.end(), [&dims](const Dim label) {
            return dims.contains(label);
          }))
        items.emplace(item.first, makeProxyItem(&item.second));
    }
  } else {
    // Shadow all global coordinates that depend on the sparse dimension.
    for (auto &item : coords) {
      const auto &labels = item.second.dimensions().labels();
      if (std::all_of(labels.begin(), labels.end(), [&dims](const Dim label) {
            return dims.contains(label);
          }))
        if (!item.second.dimensions().contains(sparseDim))
          items.emplace(item.first, makeProxyItem(&item.second));
    }
    if (sparse) {
      if constexpr (std::is_same_v<T2, void>) {
      } else if constexpr (std::is_same_v<T2, const Variable> ||
                           std::is_same_v<T2, Variable>) {
        items.emplace(sparseDim, makeProxyItem(&*sparse));
      } else {
        for (const auto &item : *sparse)
          items.emplace(item.first, makeProxyItem(&item.second));
      }
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

/// Return a const proxy to data and coordinates with given name.
DataConstProxy Dataset::operator[](const std::string &name) const {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::runtime_error("Could not find data with name " + name + ".");
  return DataConstProxy(*this, it->second);
}

/// Return a proxy to data and coordinates with given name.
DataProxy Dataset::operator[](const std::string &name) {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::runtime_error("Could not find data with name " + name + ".");
  return DataProxy(*this, it->second);
}

/// Set (insert or replace) the coordinate for the given dimension.
void Dataset::setCoord(const Dim dim, Variable coord) {
  m_coords.insert_or_assign(dim, std::move(coord));
}

/// Set (insert or replace) the labels for the given label name.
///
/// Note that the label name has no relation to names of data items.
void Dataset::setLabels(const std::string &labelName, Variable labels) {
  m_labels.insert_or_assign(labelName, std::move(labels));
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

} // namespace scipp::core::next
