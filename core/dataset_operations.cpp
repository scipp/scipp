// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/transform.h"

#include "dataset_operations_common.h"
#include "histogram.h"

namespace scipp::core {

// For now this implementation is only for the simplest case of 2 dims (inner
// stands for sparse)
DataArray histogram(const DataConstProxy &sparse,
                    const VariableConstProxy &binEdges) {
  if (sparse.hasData())
    throw except::SparseDataError(
        "`histogram` is not implemented for sparse data with values yet.");
  if (sparse.dims().ndim() > 1)
    throw std::logic_error("Only the simple case histograms may be constructed "
                           "for now: 2 dims including sparse.");
  auto dim = binEdges.dims().inner();
  if (binEdges.unit() != sparse.coords()[dim].unit())
    throw std::logic_error(
        "Bin edges must have same unit as the sparse input coordinate.");
  if (binEdges.dtype() != dtype<double> ||
      sparse.coords()[dim].dtype() != DType::Double)
    throw std::logic_error("Histogram is only available for double type.");

  auto result = apply_and_drop_dim(
      sparse,
      [](const DataConstProxy &_sparse, const Dim _dim,
         const VariableConstProxy &_binEdges) {
        auto coord = _sparse.coords()[_dim];
        auto edgesSpan = _binEdges.values<double>();
        expect::histogram::sorted_edges(edgesSpan);
        auto resDims{_sparse.dims()};
        auto len = _binEdges.dims()[_dim] - 1;
        resDims.resize(resDims.index(_dim), len);
        Variable res =
            makeVariableWithVariances<double>(resDims, units::counts);
        for (scipp::index i = 0; i < _sparse.dims().volume(); ++i) {
          const auto &coord_i = coord.sparseValues<double>()[i];
          auto curRes = res.values<double>().begin() + i * len;
          for (const auto &c : coord_i) {
            auto it = std::upper_bound(edgesSpan.begin(), edgesSpan.end(), c);
            if (it != edgesSpan.end() && it != edgesSpan.begin())
              ++(*(curRes + (--it - edgesSpan.begin())));
          }
        }
        std::copy(res.values<double>().begin(), res.values<double>().end(),
                  res.variances<double>().begin());
        return res;
      },
      dim, binEdges);
  result.setCoord(dim, binEdges);
  return result;
}

DataArray histogram(const DataConstProxy &sparse, const Variable &binEdges) {
  return histogram(sparse, VariableConstProxy(binEdges));
}

Dataset histogram(const Dataset &dataset, const VariableConstProxy &bins) {
  auto out(Dataset(DatasetConstProxy::makeProxyWithEmptyIndexes(dataset)));
  out.setCoord(bins.dims().inner(), bins);
  for (const auto &[name, item] : dataset) {
    if (item.dims().sparse())
      out.setData(std::string(name), histogram(item, bins));
  }
  return out;
}

Dataset histogram(const Dataset &dataset, const Variable &bins) {
  return histogram(dataset, VariableConstProxy(bins));
}

Dataset histogram(const Dataset &dataset, const Dim &dim) {
  auto bins = dataset.coords()[dim];
  return histogram(dataset, bins);
}

Dataset merge(const DatasetConstProxy &a, const DatasetConstProxy &b) {
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
Variable join_edges(const VariableConstProxy &a, const VariableConstProxy &b,
                    const Dim dim) {
  expect::equals(a.slice({dim, a.dims()[dim] - 1}), b.slice({dim, 0}));
  return concatenate(a.slice({dim, 0, a.dims()[dim] - 1}), b, dim);
}

/// Return the dimension for given coord or labels.
///
/// For coords, this is the same as the key, for labels we adopt the convention
/// that labels are "labelling" their inner dimension.
template <class T, class Key>
Dim dim_of_coord_or_labels(const T &dict, const Key &key) {
  if constexpr (std::is_same_v<Key, Dim>)
    return key;
  else
    return dict[key].dims().inner();
}

namespace {
template <class T1, class T2>
auto concat(const T1 &a, const T2 &b, const Dim dim, const Dimensions &dimsA,
            const Dimensions &dimsB) {
  std::map<typename T1::key_type, typename T1::mapped_type> out;
  for (const auto &[key, a_] : a) {
    if (dim_of_coord_or_labels(a, key) == dim) {
      if ((a_.dims()[dim] == dimsA[dim]) != (b[key].dims()[dim] == dimsB[dim]))
        throw except::BinEdgeError(
            "Either both or neither of the inputs must be bin edges.");
      if (a_.dims()[dim] == dimsA[dim])
        out.emplace(key, concatenate(a_, b[key], dim));
      else
        out.emplace(key, join_edges(a_, b[key], dim));
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

DataArray concatenate(const DataConstProxy &a, const DataConstProxy &b,
                      const Dim dim) {
  if (!a.dims().contains(dim) && a == b)
    return DataArray{a};
  return DataArray(concatenate(a.data(), b.data(), dim),
                   concat(a.coords(), b.coords(), dim, a.dims(), b.dims()),
                   concat(a.labels(), b.labels(), dim, a.dims(), b.dims()),
                   concat(a.masks(), b.masks(), dim, a.dims(), b.dims()));
}

Dataset concatenate(const DatasetConstProxy &a, const DatasetConstProxy &b,
                    const Dim dim) {
  Dataset result;
  for (const auto &[name, item] : a)
    if (b.contains(name))
      result.setData(name, concatenate(item, b[name], dim));
  return result;
}

DataArray sum(const DataConstProxy &a, const Dim dim) {
  return apply_to_data_and_drop_dim(a, [](auto &&... _) { return sum(_...); },
                                    dim);
}

Dataset sum(const DatasetConstProxy &d, const Dim dim) {
  // Currently not supporting sum/mean of dataset if one or more items do not
  // depend on the input dimension. The definition is ambiguous (return
  // unchanged, vs. compute sum of broadcast) so it is better to avoid this for
  // now.
  return apply_to_items(d, [](auto &&... _) { return sum(_...); }, dim);
}

DataArray mean(const DataConstProxy &a, const Dim dim) {
  return apply_to_data_and_drop_dim(a, [](auto &&... _) { return mean(_...); },
                                    dim);
}

Dataset mean(const DatasetConstProxy &d, const Dim dim) {
  return apply_to_items(d, [](auto &&... _) { return mean(_...); }, dim);
}

DataArray rebin(const DataConstProxy &a, const Dim dim,
                const VariableConstProxy &coord) {
  auto rebinned = apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return rebin(_...); }, dim, a.coords()[dim], coord);
  rebinned.setCoord(dim, coord);
  return rebinned;
}

Dataset rebin(const DatasetConstProxy &d, const Dim dim,
              const VariableConstProxy &coord) {
  return apply_to_items(d, [](auto &&... _) { return rebin(_...); }, dim,
                        coord);
}

/// Return one of the inputs if they are the same, throw otherwise.
VariableConstProxy same(const VariableConstProxy &a,
                        const VariableConstProxy &b) {
  expect::equals(a, b);
  return a;
}

/// Return a deep copy of a DataArray or of a DataProxy.
DataArray copy(const DataConstProxy &array) { return DataArray(array); }

/// Return a deep copy of a Dataset or of a DatasetProxy.
Dataset copy(const DatasetConstProxy &dataset) { return Dataset(dataset); }

} // namespace scipp::core
