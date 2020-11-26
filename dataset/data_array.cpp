// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset.h"
#include "scipp/variable/misc_operations.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

DataArray::DataArray(const DataArrayConstView &view,
                     const AttrPolicy attrPolicy)
    : DataArray(Variable(view.data()), copy_map(view.coords()),
                copy_map(view.masks()),
                attrPolicy == AttrPolicy::Keep ? copy_map(view.attrs())
                                               : std::map<Dim, Variable>{},
                view.name()) {}

DataArray::operator DataArrayConstView() const { return get(); }
DataArray::operator DataArrayView() { return get(); }

void requireValid(const DataArray &a) {
  if (!a)
    throw std::runtime_error("Invalid DataArray.");
}

const VariableConstView &DataArrayConstView::data() const { return m_view; }

const VariableView &DataArrayView::data() const { return m_view; }

DataArrayConstView DataArray::get() const {
  requireValid(*this);
  auto view = *m_holder.begin();
  view.m_isItem = false;
  return view;
}

DataArrayView DataArray::get() {
  requireValid(*this);
  auto view = *m_holder.begin();
  view.m_isItem = false;
  return view;
}

/// Return true if the dataset proxies have identical content.
bool operator==(const DataArrayConstView &a, const DataArrayConstView &b) {
  if (a.hasVariances() != b.hasVariances())
    return false;
  if (a.coords() != b.coords())
    return false;
  if (a.masks() != b.masks())
    return false;
  if (a.attrs() != b.attrs())
    return false;
  return a.data() == b.data();
}

bool operator!=(const DataArrayConstView &a, const DataArrayConstView &b) {
  return !operator==(a, b);
}

DataArray astype(const DataArrayConstView &var, const DType type) {
  return DataArray(astype(var.data(), type), var.coords(), var.masks(),
                   var.attrs());
}

} // namespace scipp::dataset
