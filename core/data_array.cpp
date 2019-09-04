// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"

namespace scipp::core {

DataArray::DataArray(const DataConstProxy &proxy) {
  m_holder.setData(proxy.name(), proxy);
}

DataArray::DataArray(std::optional<Variable> data,
                     std::map<Dim, Variable> coords,
                     std::map<std::string, Variable> labels,
                     std::map<std::string, Variable> attrs,
                     const std::string &name) {
  if (data)
    m_holder.setData(name, std::move(*data));
  for (auto & [ dim, c ] : coords)
    if (c.dims().sparse())
      m_holder.setSparseCoord(name, std::move(c));
    else
      m_holder.setCoord(dim, std::move(c));
  for (auto & [ label_name, l ] : labels)
    if (l.dims().sparse())
      m_holder.setSparseLabels(name, label_name, std::move(l));
    else
      m_holder.setLabels(label_name, std::move(l));
  for (auto & [ attr_name, a ] : attrs)
    m_holder.setAttr(attr_name, std::move(a));
}

DataArray::operator DataConstProxy() const { return get(); }
DataArray::operator DataProxy() { return get(); }

DataArray &DataArray::operator+=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() += other.data();
  return *this;
}

DataArray &DataArray::operator-=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() -= other.data();
  return *this;
}

DataArray &DataArray::operator*=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() *= other.data();
  return *this;
}

DataArray &DataArray::operator/=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() /= other.data();
  return *this;
}

DataArray &DataArray::operator+=(const Variable &other) {
  data() += other;
  return *this;
}

DataArray &DataArray::operator-=(const Variable &other) {
  data() -= other;
  return *this;
}

DataArray &DataArray::operator*=(const Variable &other) {
  data() *= other;
  return *this;
}

DataArray &DataArray::operator/=(const Variable &other) {
  data() /= other;
  return *this;
}

DataArray operator+(const DataConstProxy &a, const DataConstProxy &b) {
  return DataArray(a.data() + b.data(), union_(a.coords(), b.coords()),
                   union_(a.labels(), b.labels()));
}

DataArray operator-(const DataConstProxy &a, const DataConstProxy &b) {
  return {a.data() - b.data(), union_(a.coords(), b.coords()),
          union_(a.labels(), b.labels())};
}

DataArray operator*(const DataConstProxy &a, const DataConstProxy &b) {
  return {a.data() * b.data(), union_(a.coords(), b.coords()),
          union_(a.labels(), b.labels())};
}

DataArray operator/(const DataConstProxy &a, const DataConstProxy &b) {
  return {a.data() / b.data(), union_(a.coords(), b.coords()),
          union_(a.labels(), b.labels())};
}

} // namespace scipp::core
