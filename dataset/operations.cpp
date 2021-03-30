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

auto union_(const Dataset &a, const Dataset &b) {
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

Dataset merge(const Dataset &a, const Dataset &b) {
  return Dataset(union_(a, b), union_(a.coords(), b.coords()));
}

/// Return a deep copy of a DataArray.
DataArray copy(const DataArrayConstView &array, const AttrPolicy attrPolicy) {
  // TODO is this correct? copy data but not meta data?
  return DataArray(copy(array.data()), array.coords(), array.masks(),
                   attrPolicy == AttrPolicy::Keep ? array.attrs() : Attrs{},
                   array.name());
}

/// Return a deep copy of a Dataset.
Dataset copy(const DatasetConstView &dataset, const AttrPolicy attrPolicy) {
  Dataset out({}, dataset.coords());
  for (const auto &item : dataset)
    out.setData(item.name(), copy(item, attrPolicy));
  return out;
}

namespace {
template <class T>
void copy_item(const DataArrayConstView &from, T &&to,
               const AttrPolicy attrPolicy) {
  for (const auto &[name, mask] : from.masks())
    copy(mask, to.masks()[name]);
  if (attrPolicy == AttrPolicy::Keep)
    for (const auto [dim, attr] : from.attrs())
      copy(attr, to.attrs()[dim]);
  copy(from.data(), to.data());
}
} // namespace

/// Copy data array to output data array
DataArray &copy(const DataArray &array, DataArray &out,
                const AttrPolicy attrPolicy) {
  // TODO self assignment check?
  // TODO do we need something like
  // expect::coordsAreSuperset(*this, other);
  for (const auto [dim, coord] : array.coords())
    copy(coord, out.coords()[dim]);
  copy_item(array, out, attrPolicy);
  return out;
}

/// Copy data array to output data array
DataArray copy(const DataArray &array, DataArray &&out,
               const AttrPolicy attrPolicy) {
  copy(array, out, attrPolicy);
  return std::move(out);
}

/// Copy dataset to output dataset
Dataset &copy(const Dataset &dataset, Dataset &out,
              const AttrPolicy attrPolicy) {
  for (const auto [dim, coord] : dataset.coords())
    copy(coord, out.coords()[dim]);
  for (const auto &array : dataset)
    copy_item(array, out[array.name()], attrPolicy);
  return out;
}

/// Copy dataset to output dataset
Dataset copy(const Dataset &dataset, Dataset &&out,
             const AttrPolicy attrPolicy) {
  copy(dataset, out, attrPolicy);
  return std::move(out);
}

} // namespace scipp::dataset
