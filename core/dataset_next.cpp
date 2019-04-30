// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dataset_next.h"
#include "dataset.h"
#include "except.h"

namespace scipp::core::next {

/// Return a proxy to all coordinates of the dataset.
///
/// This proxy includes only "dimension-coordinates". To access
/// non-dimension-coordinates" see labels().
CoordsConstProxy Dataset::coords() const noexcept {
  return CoordsConstProxy(*this);
}

/// Return a proxy to all labels of the dataset.
LabelsConstProxy Dataset::labels() const noexcept {
  return LabelsConstProxy(*this);
}

/// Return a proxy to data and coordinates with given name.
DataConstProxy Dataset::operator[](const std::string &name) const {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::runtime_error("Could not find data with name " + name + ".");
  return DataConstProxy(*this, it->second);
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
    return m_data->values->dimensions();
  return m_data->coord->dimensions();
}

/// Return an ordered range of dimension extents, excluding a potential sparse
/// dimension.
///
/// The first item in the range corresponds to the outermost dimension and the
/// last item corresponds to the inntermost dimension of the underlying data.
scipp::span<const index> DataConstProxy::shape() const noexcept {
  if (hasValues())
    return m_data->values->dimensions().shape();
  return m_data->coord->dimensions().shape();
}

/// Return the unit of the data values.
///
/// Throws if there are no data values.
units::Unit DataConstProxy::unit() const {
  if (hasValues())
    return values().unit();
  throw std::runtime_error("Data without values, unit is undefined.");
}

/// Return a proxy to all coordinates of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's coordinates that depends on the sparse dimension.
CoordsConstProxy DataConstProxy::coords() const noexcept {
  if (!isSparse())
    return CoordsConstProxy(*m_dataset);
  return CoordsConstProxy(*m_dataset, sparseDim(), m_data->coord);
}

/// Return a proxy to all labels of the data proxy.
///
/// If the data has a sparse dimension the returned proxy will not contain any
/// of the dataset's labels that depends on the sparse dimension.
LabelsConstProxy DataConstProxy::labels() const noexcept {
  if (!isSparse())
    return LabelsConstProxy(*m_dataset);
  return LabelsConstProxy(*m_dataset, sparseDim(), &m_data->labels);
}

} // namespace scipp::core::next
