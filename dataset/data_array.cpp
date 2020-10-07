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

/*
/// Return the bounds of all sliced realigned dimensions.
std::vector<std::pair<Dim, Variable>> DataArrayConstView::slice_bounds() const {
  std::vector<std::pair<Dim, Variable>> bounds;
  std::map<Dim, std::pair<scipp::index, scipp::index>> combined_slices;
  for (const auto &item : slices()) {
    const auto s = item.first;
    const auto dim = s.dim();
    // Only process realigned dims
    if (unaligned().dims().contains(dim) ||
        !unaligned().aligned_coords().contains(dim))
      continue;
    const auto left = s.begin();
    const auto right = s.end() == -1 ? left + 1 : s.end();
    if (combined_slices.count(dim)) {
      combined_slices[dim].second = combined_slices[dim].first + right;
      combined_slices[dim].first += left;
    } else {
      combined_slices[dim] = {left, right};
    }
  }
  for (const auto &[dim, interval] : combined_slices) {
    const auto [left, right] = interval;
    const auto coord = m_dataset->coords()[dim];
    bounds.emplace_back(dim, concatenate(coord.slice({dim, left}),
                                         coord.slice({dim, right}), dim));
  }
  // TODO As an optimization we could sort the bounds and put those that slice
  // out the largest fraction first, to ensure that `filter_recurse` slices in a
  // potentially faster way.
  return bounds;
}
*/

namespace {

template <class... DataArgs>
auto makeDataArray(const DataArrayConstView &view, const AttrPolicy attrPolicy,
                   DataArgs &&... dataArgs) {
  return DataArray(std::forward<DataArgs>(dataArgs)...,
                   copy_map(view.aligned_coords()), copy_map(view.masks()),
                   attrPolicy == AttrPolicy::Keep
                       ? copy_map(view.unaligned_coords())
                       : std::map<Dim, Variable>{},
                   view.name());
}

} // namespace

// TODO Should bucket variables filter on copy? See unaligned::filter_recurse
DataArray::DataArray(const DataArrayConstView &view,
                     const AttrPolicy attrPolicy)
    : DataArray(makeDataArray(view, attrPolicy, Variable(view.data()))) {}

DataArray::operator DataArrayConstView() const { return get(); }
DataArray::operator DataArrayView() { return get(); }

void requireValid(const DataArray &a) {
  if (!a)
    throw std::runtime_error("Invalid DataArray.");
}

const VariableConstView &DataArrayConstView::data() const {
  return m_view;
}

const VariableView &DataArrayView::data() const {
  return m_view;
}

DataArrayConstView DataArray::get() const {
  requireValid(*this);
  return *m_holder.begin();
}

DataArrayView DataArray::get() {
  requireValid(*this);
  return *m_holder.begin();
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
