// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/shape.h"

#include "scipp/dataset/except.h"
#include "scipp/dataset/shape.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

namespace scipp::dataset {

/// Return one of the inputs if they are the same, throw otherwise.
template <class T> T same(const T &a, const T &b) {
  core::expect::equals(a, b);
  return a;
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
constexpr auto is_bin_edges = [](const auto &coord, const auto &dims,
                                 const Dim dim) {
  return coord.dims().contains(dim) &&
         ((dims.count(dim) == 1) ? coord.dims()[dim] != dims.at(dim)
                                 : coord.dims()[dim] == 2);
};
template <class T1, class T2, class DimT>
auto concat(const T1 &a, const T2 &b, const Dim dim, const DimT &dimsA,
            const DimT &dimsB) {
  std::map<typename T1::key_type, typename T1::mapped_type> out;
  for (const auto &[key, a_] : a) {
    if (dim_of_coord(a_, key) == dim) {
      if (is_bin_edges(a_, dimsA, dim) != is_bin_edges(b[key], dimsB, dim)) {
        throw except::BinEdgeError(
            "Either both or neither of the inputs must be bin edges.");
      } else if (a_.dims()[dim] ==
                 ((dimsA.count(dim) == 1) ? dimsA.at(dim) : 1)) {
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
  auto out = DataArray(a.hasData() || b.hasData()
                           ? concatenate(a.data(), b.data(), dim)
                           : Variable{},
                       {},
                       concat(a.masks(), b.masks(), dim, a.dims(), b.dims()));
  for (auto &&[d, coord] :
       concat(a.coords(), b.coords(), dim, a.dims(), b.dims())) {
    if (d == dim || a.aligned_coords().contains(d) ||
        b.aligned_coords().contains(d))
      out.coords().set(d, std::move(coord));
    else
      out.unaligned_coords().set(d, std::move(coord));
  }
  return out;
}

Dataset concatenate(const DatasetConstView &a, const DatasetConstView &b,
                    const Dim dim) {
  auto result = a.empty() ? Dataset(std::map<std::string, Variable>(),
                                    concat(a.coords(), b.coords(), dim,
                                           a.dimensions(), b.dimensions()))
                          : Dataset();
  for (const auto &item : a)
    if (b.contains(item.name())) {
      if (!item.dims().contains(dim) && item == b[item.name()])
        result.setData(item.name(), item);
      else
        result.setData(item.name(), concatenate(item, b[item.name()], dim));
    }
  return result;
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

} // namespace scipp::dataset
