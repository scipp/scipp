// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"

namespace scipp::core {

DataArray::DataArray(const DataConstProxy &proxy) {
  m_holder.setData(proxy.name(), proxy);
}

DataArray::DataArray(Variable data, std::map<Dim, Variable> coords,
                     std::map<std::string, Variable> labels) {
  m_holder.setData("", std::move(data));
  for (auto & [ dim, c ] : coords)
    if (c.dims().sparse())
      m_holder.setSparseCoord("", std::move(c));
    else
      m_holder.setCoord(dim, std::move(c));
  for (auto & [ name, l ] : labels)
    if (l.dims().sparse())
      m_holder.setSparseLabels("", name, std::move(l));
    else
      m_holder.setLabels(name, std::move(l));
}

DataArray::operator DataConstProxy() const { return get(); }
DataArray::operator DataProxy() { return get(); }

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
