// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"

#include "dataset_operations_common.h"
#include "variable_operations_common.h"

namespace scipp::core {

auto union_(const DatasetConstView &a, const DatasetConstView &b) {
  std::map<std::string, DataArray> out;

  for (const auto &item : a)
    out.emplace(item.name(), item);
  for (const auto &item : b) {
    if (const auto it = a.find(item.name()); it != a.end())
      expect::equals(item, *it);
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
                 union_(a.labels(), b.labels()), union_(a.masks(), b.masks()),
                 union_(a.attrs(), b.attrs()));
}

/// Concatenate a and b, assuming that a and b contain bin edges.
///
/// Checks that the last edges in `a` match the first edges in `b`. The
/// Concatenates the input edges, removing duplicate bin edges.
Variable join_edges(const VariableConstView &a, const VariableConstView &b,
                    const Dim dim) {
  expect::equals(a.slice({dim, a.dims()[dim] - 1}), b.slice({dim, 0}));
  return concatenate(a.slice({dim, 0, a.dims()[dim] - 1}), b, dim);
}

namespace {
template <class T1, class T2, class DimT>
auto concat(const T1 &a, const T2 &b, const Dim dim, const DimT &dimsA,
            const DimT &dimsB) {
  std::map<typename T1::key_type, typename T1::mapped_type> out;
  for (const auto &[key, a_] : a) {
    if (dim_of_coord_or_labels(a_, key) == dim) {
      if (a_.dims().sparseDim() == dim) {
        if (b[key].dims().sparseDim() == dim)
          out.emplace(key, concatenate(a_, b[key], dim));
        else
          throw except::SparseDataError("Either both or neither of the inputs "
                                        "must be sparse in given dim.");
      } else if ((a_.dims()[dim] == dimsA.at(dim)) !=
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
                       : std::optional<Variable>(),
                   concat(a.coords(), b.coords(), dim, a.dims(), b.dims()),
                   concat(a.labels(), b.labels(), dim, a.dims(), b.dims()),
                   concat(a.masks(), b.masks(), dim, a.dims(), b.dims()));
}

Dataset concatenate(const DatasetConstView &a, const DatasetConstView &b,
                    const Dim dim) {
  Dataset result(
      std::map<std::string, Variable>(),
      concat(a.coords(), b.coords(), dim, a.dimensions(), b.dimensions()),
      concat(a.labels(), b.labels(), dim, a.dimensions(), b.dimensions()),
      concat(a.masks(), b.masks(), dim, a.dimensions(), b.dimensions()),
      std::map<std::string, Variable>());
  for (const auto &item : a)
    if (b.contains(item.name()))
      result.setData(item.name(), concatenate(item, b[item.name()], dim));
  return result;
}

DataArray flatten(const DataArrayConstView &a, const Dim dim) {
  return apply_or_copy_dim(a, [](auto &&... _) { return flatten(_...); }, dim,
                           a.masks());
}

Dataset flatten(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(d, [](auto &&... _) { return flatten(_...); }, dim);
}

DataArray sum(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(a, [](auto &&... _) { return sum(_...); },
                                    dim, a.masks());
}

Dataset sum(const DatasetConstView &d, const Dim dim) {
  // Currently not supporting sum/mean of dataset if one or more items do not
  // depend on the input dimension. The definition is ambiguous (return
  // unchanged, vs. compute sum of broadcast) so it is better to avoid this for
  // now.
  return apply_to_items(d, [](auto &&... _) { return sum(_...); }, dim);
}

DataArray mean(const DataArrayConstView &a, const Dim dim) {
  return apply_to_data_and_drop_dim(a, [](auto &&... _) { return mean(_...); },
                                    dim, a.masks());
}

Dataset mean(const DatasetConstView &d, const Dim dim) {
  return apply_to_items(d, [](auto &&... _) { return mean(_...); }, dim);
}

DataArray rebin(const DataArrayConstView &a, const Dim dim,
                const VariableConstView &coord) {
  auto rebinned = apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return rebin(_...); }, dim, a.coords()[dim], coord);

  for (auto &&[name, mask] : a.masks()) {
    if (mask.dims().contains(dim))
      rebinned.masks().set(name, rebin(mask, dim, a.coords()[dim], coord));
  }

  rebinned.setCoord(dim, coord);
  return rebinned;
}

Dataset rebin(const DatasetConstView &d, const Dim dim,
              const VariableConstView &coord) {
  return apply_to_items(d, [](auto &&... _) { return rebin(_...); }, dim,
                        coord);
}

DataArray resize(const DataArrayConstView &a, const Dim dim,
                 const scipp::index size) {
  if (a.dims().sparse()) {
    const auto resize_if_sparse = [dim, size](const auto &var) {
      return var.dims().sparse() ? resize(var, dim, size) : Variable{var};
    };

    std::map<Dim, Variable> coords;
    for (auto &&[d, coord] : a.coords())
      if (dim_of_coord_or_labels(coord, d) != dim)
        coords.emplace(d, resize_if_sparse(coord));

    std::map<std::string, Variable> labels;
    for (auto &&[name, label] : a.labels())
      if (label.dims().inner() != dim)
        labels.emplace(name, resize_if_sparse(label));

    std::map<std::string, Variable> attrs;
    for (auto &&[name, attr] : a.attrs())
      if (attr.dims().inner() != dim)
        attrs.emplace(name, resize_if_sparse(attr));

    std::map<std::string, Variable> masks;
    for (auto &&[name, mask] : a.masks())
      if (mask.dims().inner() != dim)
        masks.emplace(name, resize_if_sparse(mask));

    return DataArray{a.hasData() ? resize(a.data(), dim, size)
                                 : std::optional<Variable>{},
                     std::move(coords), std::move(labels), std::move(masks),
                     std::move(attrs)};
  } else {
    return apply_to_data_and_drop_dim(
        a, [](auto &&... _) { return resize(_...); }, dim, size);
  }
}

Dataset resize(const DatasetConstView &d, const Dim dim,
               const scipp::index size) {
  return apply_to_items(d, [](auto &&... _) { return resize(_...); }, dim,
                        size);
}

/// Return one of the inputs if they are the same, throw otherwise.
VariableConstView same(const VariableConstView &a, const VariableConstView &b) {
  expect::equals(a, b);
  return a;
}

/// Return a deep copy of a DataArray or of a DataArrayView.
DataArray copy(const DataArrayConstView &array) { return DataArray(array); }

/// Return a deep copy of a Dataset or of a DatasetView.
Dataset copy(const DatasetConstView &dataset) { return Dataset(dataset); }

} // namespace scipp::core
