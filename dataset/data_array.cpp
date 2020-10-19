// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/common/numeric.h"

#include "scipp/core/histogram.h"

#include "scipp/variable/event.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/histogram.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

DataArray::DataArray(const DataArrayConstView &view,
                     const AttrPolicy attrPolicy)
    : DataArray(Variable(view.data()), copy_map(view.aligned_coords()),
                copy_map(view.masks()),
                attrPolicy == AttrPolicy::Keep
                    ? copy_map(view.unaligned_coords())
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
  return a.data() == b.data();
}

bool operator!=(const DataArrayConstView &a, const DataArrayConstView &b) {
  return !operator==(a, b);
}

DataArray astype(const DataArrayConstView &var, const DType type) {
  return DataArray(astype(var.data(), type), var.aligned_coords(), var.masks(),
                   var.unaligned_coords());
}

} // namespace scipp::dataset
