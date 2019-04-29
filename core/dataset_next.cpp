// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dataset_next.h"
#include "dataset.h"
#include "except.h"

namespace scipp::core::next {

CoordsConstProxy Dataset::coords() const noexcept {
  return CoordsConstProxy(*this);
}

DataConstProxy Dataset::operator[](const std::string &name) const {
  const auto it = m_data.find(name);
  if (it == m_data.end())
    throw std::runtime_error("Could not find data with name " + name + ".");
  // Note that we are passing it->first instead of name to avoid storing a
  // reference to a temporary.
  return DataConstProxy(this, it->first);
}

void Dataset::setCoord(const Dim dim, Variable coord) {
  m_coords.insert_or_assign(dim, std::move(coord));
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

void Dataset::setSparseCoord(const std::string &name, Variable coord) {
  if (!coord.isSparse())
    throw std::runtime_error("Variable passed to Dataset::setSparseCoord does "
                             "not contain sparse data.");
  if (m_data.count(name)) {
    const auto &data = m_data.at(name);
    if ((data.values && (data.values->sparseDim() != coord.sparseDim())) ||
        (data.variances && (data.variances->sparseDim() != coord.sparseDim())))
      throw std::runtime_error("Cannot set sparse coordinate if values or "
                               "variances are not sparse.");
  }
  m_data[name].coord = std::move(coord);
}

ConstVariableSlice CoordsConstProxy::operator[](const Dim dim) const {
  return ConstVariableSlice(*m_coords.at(dim));
}

bool DataConstProxy::isSparse() const noexcept {
  if (m_data->coord)
    return true;
  if (hasValues())
    return values().isSparse();
  if (hasVariances())
    return variances().isSparse();
  return false;
}

Dim DataConstProxy::sparseDim() const noexcept {
  if (m_data->coord)
    return m_data->coord->sparseDim();
  if (hasValues())
    return values().sparseDim();
  if (hasVariances())
    return variances().sparseDim();
  return Dim::Invalid;
}

scipp::span<const Dim> DataConstProxy::dims() const noexcept {
  if (hasValues())
    return m_data->values->dimensions().labels();
  return m_data->coord->dimensions().labels();
}

scipp::span<const index> DataConstProxy::shape() const noexcept {
  if (hasValues())
    return m_data->values->dimensions().shape();
  return m_data->coord->dimensions().shape();
}

units::Unit DataConstProxy::unit() const {
  if (hasValues())
    return values().unit();
  throw std::runtime_error("Data without values, unit is undefined.");
}

CoordsConstProxy DataConstProxy::coords() const noexcept {
  if (!isSparse())
    return CoordsConstProxy(*m_dataset);
  return CoordsConstProxy(*m_dataset, sparseDim(), m_data->coord);
}

} // namespace scipp::core::next
