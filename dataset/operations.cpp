// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"

#include "scipp/variable/misc_operations.h"
#include "scipp/variable/reduction.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

auto union_(const DatasetConstView &a, const DatasetConstView &b) {
  std::map<std::string, DataArray> out;

  for (const auto &item : a)
    out.emplace(item.name(), item);
  for (const auto &item : b) {
    if (const auto it = a.find(item.name()); it != a.end())
      core::expect::equals(item, *it);
    else
      out.emplace(item.name(), item);
  }
  return out;
}

Dataset merge(const DatasetConstView &a, const DatasetConstView &b) {
  return Dataset(union_(a, b), union_(a.coords(), b.coords()));
}

/// Return a deep copy of a DataArray or of a DataArrayView.
DataArray copy(const DataArrayConstView &array, const AttrPolicy attrPolicy) {
  return DataArray(array, attrPolicy);
}

/// Return a deep copy of a Dataset or of a DatasetView.
Dataset copy(const DatasetConstView &dataset, const AttrPolicy attrPolicy) {
  if (attrPolicy != AttrPolicy::Keep)
    throw std::runtime_error(
        "Dropping attributes when copying dataset not implemented yet.");
  return Dataset(dataset);
}

namespace {
void copy_item(const DataArrayConstView &from, const DataArrayView &to,
               const AttrPolicy attrPolicy) {
  for (const auto &[name, mask] : from.masks())
    to.masks()[name].assign(mask);
  if (attrPolicy == AttrPolicy::Keep)
    for (const auto &[dim, attr] : from.attrs())
      to.attrs()[dim].assign(attr);
  to.data().assign(from.data());
}
} // namespace

/// Copy data array to output data array
DataArrayView copy(const DataArrayConstView &array, const DataArrayView &out,
                   const AttrPolicy attrPolicy) {
  for (const auto &[dim, coord] : array.coords())
    out.coords()[dim].assign(coord);
  copy_item(array, out, attrPolicy);
  return out;
}

/// Copy dataset to output dataset
DatasetView copy(const DatasetConstView &dataset, const DatasetView &out,
                 const AttrPolicy attrPolicy) {
  for (const auto &[dim, coord] : dataset.coords())
    out.coords()[dim].assign(coord);
  for (const auto &array : dataset)
    copy_item(array, out[array.name()], attrPolicy);
  return out;
}

} // namespace scipp::dataset
