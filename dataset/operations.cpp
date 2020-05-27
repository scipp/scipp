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

/// Concatenate a and b, assuming that a and b contain bin edges.
///
/// Checks that the last edges in `a` match the first edges in `b`. The
/// Concatenates the input edges, removing duplicate bin edges.
template <class View>
typename View::value_type join_edges(const View &a, const View &b,
                                     const Dim dim) {
  core::expect::equals(a.slice({dim, a.dims()[dim] - 1}), b.slice({dim, 0}));
  return concatenate(a.slice({dim, 0, a.dims()[dim] - 1}), b, dim);
}

namespace {
template <class T1, class T2, class DimT>
auto concat(const T1 &a, const T2 &b, const Dim dim, const DimT &dimsA,
            const DimT &dimsB) {
  std::map<typename T1::key_type, typename T1::mapped_type> out;
  for (const auto &[key, a_] : a) {
    if (dim_of_coord(a_, key) == dim) {
      if ((a_.dims()[dim] == dimsA.at(dim)) !=
          (b[key].dims()[dim] == dimsB.at(dim))) {
        throw except::BinEdgeError(
            "Either both or neither of the inputs must be bin edges.");
      } else if (a_.dims()[dim] == dimsA.at(dim)) {
        out.emplace(key, concatenate(a_, b[key], dim));
      } else {
        out.emplace(key, join_edges(a_, b[key], dim));
      }
    } else {
      // 1D coord is kept only if both inputs have matching 1D coords.
      if (a_.dims().contains(dim) || b[key].dims().contains(dim) ||
          a_ != b[key])
        out.emplace(key, concatenate(a_, b[key], dim));
      else
        out.emplace(key, same(a_, b[key]));
    }
  }
  return out;
}
} // namespace

DataArray concatenate(const DataArrayConstView &a, const DataArrayConstView &b,
                      const Dim dim) {
  if (!a.dims().contains(dim) && a == b)
    return DataArray{a};
  return DataArray(a.hasData() || b.hasData()
                       ? concatenate(a.data(), b.data(), dim)
                       : Variable{},
                   concat(a.coords(), b.coords(), dim, a.dims(), b.dims()),
                   concat(a.masks(), b.masks(), dim, a.dims(), b.dims()));
}

Dataset concatenate(const DatasetConstView &a, const DatasetConstView &b,
                    const Dim dim) {
  Dataset result(
      std::map<std::string, Variable>(),
      concat(a.coords(), b.coords(), dim, a.dimensions(), b.dimensions()),
      concat(a.masks(), b.masks(), dim, a.dimensions(), b.dimensions()),
      std::map<std::string, Variable>());
  for (const auto &item : a)
    if (b.contains(item.name()))
      result.setData(item.name(), concatenate(item, b[item.name()], dim));
  return result;
}

DataArray flatten(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a,
      overloaded{no_realigned_support,
                 [](const auto &x, const Dim dim_, const auto &mask_) {
                   if (!contains_events(x) && min(x, dim_) != max(x, dim_))
                     throw except::EventDataError(
                         "flatten with non-constant scalar weights not "
                         "possible yet.");
                   return contains_events(x) ? flatten(x, dim_, mask_)
                                             : copy(x.slice({dim_, 0}));
                 }},
      dim, a.masks());
}

Dataset flatten(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(
      d, [](auto &&... _) { return flatten(_...); }, dim);
}

namespace {
UnalignedData sum(Dimensions dims, const DataArrayConstView &unaligned,
                  const Dim dim, const MasksConstView &masks) {
  static_cast<void>(masks); // relevant masks are part of unaligned as well
  dims.erase(dim);
  return {dims, flatten(unaligned, dim)};
}
} // namespace

DataArray sum(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return sum(_...); }, dim, a.masks());
}

Dataset sum(const DatasetConstView &d, const Dim dim) {
  // Currently not supporting sum/mean of dataset if one or more items do not
  // depend on the input dimension. The definition is ambiguous (return
  // unchanged, vs. compute sum of broadcast) so it is better to avoid this for
  // now.
  return apply_to_items(
      d, [](auto &&... _) { return sum(_...); }, dim);
}

DataArray mean(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(
      a,
      overloaded{no_realigned_support, [](auto &&... _) { return mean(_...); }},
      dim, a.masks());
}

Dataset mean(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(
      d, [](auto &&... _) { return mean(_...); }, dim);
}

DataArray rebin(const DataArrayConstView &a, const Dim dim,
                const VariableConstView &coord) {
  auto rebinned = apply_to_data_and_drop_dim(
      a,
      overloaded{no_realigned_support,
                 [](auto &&... _) { return rebin(_...); }},
      dim, a.coords()[dim], coord);

  for (auto &&[name, mask] : a.masks()) {
    if (mask.dims().contains(dim))
      rebinned.masks().set(name, rebin(mask, dim, a.coords()[dim], coord));
  }

  rebinned.coords().set(dim, coord);
  return rebinned;
}

Dataset rebin(const DatasetConstView &d, const Dim dim,
              const VariableConstView &coord) {
  return apply_to_items(
      d, [](auto &&... _) { return rebin(_...); }, dim, coord);
}

namespace {
UnalignedData resize(Dimensions dims, const DataArrayConstView &unaligned,
                     const Dim dim, const scipp::index size) {
  dims.resize(dim, size);
  return {dims, resize(unaligned, dim, size)};
}
} // namespace

DataArray resize(const DataArrayConstView &a, const Dim dim,
                 const scipp::index size) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return resize(_...); }, dim, size);
}

Dataset resize(const DatasetConstView &d, const Dim dim,
               const scipp::index size) {
  return apply_to_items(
      d, [](auto &&... _) { return resize(_...); }, dim, size);
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
