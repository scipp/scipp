// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
  // When merging datasets the contents of the masks are not OR'ed, but
  // checked if present in both dataset with the same values with `union_`.
  // If the values are different the merge will fail.
  return Dataset(union_(a, b), union_(a.coords(), b.coords()),
                 union_(a.masks(), b.masks()), union_(a.attrs(), b.attrs()));
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
void copy_item(const DataArrayConstView &from, const DataArrayView &to) {
  if (from.hasData())
    to.data().assign(from.data());
  else
    throw except::UnalignedError(
        "Copying unaligned data to output not supported.");
}

template <class ConstView, class View>
View copy_impl(const ConstView &in, const View &out,
               const AttrPolicy attrPolicy) {
  for (const auto &[dim, coord] : in.coords())
    out.coords()[dim].assign(coord);
  for (const auto &[name, mask] : in.masks())
    out.masks()[name].assign(mask);
  if (attrPolicy == AttrPolicy::Keep)
    for (const auto &[name, attr] : in.attrs())
      out.attrs()[name].assign(attr);

  if constexpr (std::is_same_v<View, DatasetView>) {
    for (const auto &array : in) {
      copy_item(array, out[array.name()]);
      if (attrPolicy == AttrPolicy::Keep)
        for (const auto &[name, attr] : array.attrs())
          out[array.name()].attrs()[name].assign(attr);
    }
  } else {
    copy_item(in, out);
  }

  return out;
}
} // namespace

/// Copy data array to output data array
DataArrayView copy(const DataArrayConstView &array, const DataArrayView &out,
                   const AttrPolicy attrPolicy) {
  return copy_impl(array, out, attrPolicy);
}

/// Copy dataset to output dataset
DatasetView copy(const DatasetConstView &dataset, const DatasetView &out,
                 const AttrPolicy attrPolicy) {
  return copy_impl(dataset, out, attrPolicy);
}

} // namespace scipp::dataset
