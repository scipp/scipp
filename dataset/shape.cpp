// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/shape.h"
#include "scipp/variable/creation.h"

#include "scipp/dataset/except.h"
#include "scipp/dataset/shape.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"

using namespace scipp::variable;

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
Variable join_edges(const View &a, const View &b, const Dim dim) {
  core::expect::equals(a.slice({dim, a.dims()[dim] - 1}), b.slice({dim, 0}));
  return concatenate(a.slice({dim, 0, a.dims()[dim] - 1}), b, dim);
}

namespace {
template <class T1, class T2, class DimT>
auto concat(const T1 &a, const T2 &b, const Dim dim, const DimT &dimsA,
            const DimT &dimsB) {
  std::unordered_map<typename T1::key_type, typename T1::mapped_type> out;
  for (const auto [key, a_] : a) {
    if (a.dim_of(key) == dim) {
      if (is_edges(dimsA, a_.dims(), dim) !=
          is_edges(dimsB, b[key].dims(), dim)) {
        throw except::BinEdgeError(
            "Either both or neither of the inputs must be bin edges.");
      } else if (a_.dims()[dim] == (dimsA.contains(dim) ? dimsA.at(dim) : 1)) {
        out.emplace(key, concatenate(a_, b[key], dim));
      } else {
        out.emplace(key, join_edges(a_, b[key], dim));
      }
    } else {
      // 1D coord is kept only if both inputs have matching 1D coords.
      if (a_.dims().contains(dim) || b[key].dims().contains(dim) ||
          a_ != b[key])
        // Mismatching 1D coords must be broadcast to ensure new coord shape
        // matches new data shape.
        out.emplace(
            key,
            concatenate(
                broadcast(a_, merge(dimsA.contains(dim)
                                        ? Dimensions(dim, dimsA.at(dim))
                                        : Dimensions(),
                                    a_.dims())),
                broadcast(b[key], merge(dimsB.contains(dim)
                                            ? Dimensions(dim, dimsB.at(dim))
                                            : Dimensions(),
                                        b[key].dims())),
                dim));
      else
        out.emplace(key, same(a_, b[key]));
    }
  }
  return out;
}
} // namespace

DataArray concatenate(const DataArray &a, const DataArray &b, const Dim dim) {
  auto out = DataArray(concatenate(a.data(), b.data(), dim), {},
                       concat(a.masks(), b.masks(), dim, a.dims(), b.dims()));
  for (auto &&[d, coord] :
       concat(a.meta(), b.meta(), dim, a.dims(), b.dims())) {
    if (d == dim || a.coords().contains(d) || b.coords().contains(d))
      out.coords().set(d, std::move(coord));
    else
      out.attrs().set(d, std::move(coord));
  }
  return out;
}

Dataset concatenate(const Dataset &a, const Dataset &b, const Dim dim) {
  // Note that in the special case of a dataset without data items (only coords)
  // concatenating a range slice with a non-range slice will fail due to the
  // missing unaligned coord in the non-range slice. This is an extremely
  // special case and cannot be handled without adding support for unaligned
  // coords to dataset (which is not desirable for a variety of reasons). It is
  // unlikely that this will cause trouble in practice. Users can just use a
  // range slice of thickness 1.
  Dataset result;
  if (a.empty())
    result.setCoords(
        Coords(concatenate(a.sizes(), b.sizes(), dim),
               concat(a.coords(), b.coords(), dim, a.sizes(), b.sizes())));
  for (const auto &item : a)
    if (b.contains(item.name())) {
      if (!item.dims().contains(dim) && item == b[item.name()])
        result.setData(item.name(), item);
      else
        result.setData(item.name(), concatenate(item, b[item.name()], dim));
    }
  return result;
}

DataArray resize(const DataArray &a, const Dim dim, const scipp::index size,
                 const FillValue fill) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&... _) { return resize(_...); }, dim, size, fill);
}

Dataset resize(const Dataset &d, const Dim dim, const scipp::index size,
               const FillValue fill) {
  return apply_to_items(
      d, [](auto &&... _) { return resize(_...); }, dim, size, fill);
}

DataArray resize(const DataArray &a, const Dim dim, const DataArray &shape) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&v, const Dim, auto &&s) { return resize(v, s); }, dim,
      shape.data());
}

Dataset resize(const Dataset &d, const Dim dim, const Dataset &shape) {
  Dataset result;
  for (const auto &data : d)
    result.setData(data.name(), resize(data, dim, shape[data.name()]));
  return result;
}

namespace {

/// Either broadcast variable to from_dims before a reshape or not:
///
/// 1. If all from_dims are contained in the variable's dims, no broadcast
/// 2. If at least one (but not all) of the from_dims is contained in the
///    variable's dims, broadcast
/// 3. If none of the variables's dimensions are contained, no broadcast
Variable maybe_broadcast(const Variable &var,
                         const scipp::span<const Dim> &from_labels,
                         const Dimensions &data_dims) {
  const auto &var_dims = var.dims();
  Dimensions broadcast_dims;
  for (const auto &dim : var_dims.labels())
    if (std::find(from_labels.begin(), from_labels.end(), dim) ==
        from_labels.end())
      broadcast_dims.addInner(dim, var_dims[dim]);
    else
      for (const auto &lab : from_labels)
        if (!broadcast_dims.contains(lab)) {
          // Need to check if the variable contains that dim, and use the
          // variable shape in case we have a bin edge.
          if (var_dims.contains(lab))
            broadcast_dims.addInner(lab, var_dims[lab]);
          else
            broadcast_dims.addInner(lab, data_dims[lab]);
        }
  return broadcast(var, broadcast_dims);
}

/// Special handling for folding coord along a dim that contains bin edges.
Variable fold_bin_edge(const Variable &var, const Dim from_dim,
                       const Dimensions &to_dims) {
  auto out = var.slice({from_dim, 0, var.dims()[from_dim] - 1})
                 .fold(from_dim, to_dims) // fold non-overlapping part
                 .as_const();             // mark readonly since we add overlap
  // Increase dims without changing strides to obtain first == last
  out.unchecked_dims().resize(to_dims.inner(), to_dims[to_dims.inner()] + 1);
  return out;
}

/// Special handling for flattening coord along a dim that contains bin edges.
Variable flatten_bin_edge(const Variable &var,
                          const scipp::span<const Dim> &from_labels,
                          const Dim to_dim, const Dim bin_edge_dim) {
  const auto data_shape = var.dims()[bin_edge_dim] - 1;

  // Make sure that the bin edges can be joined together
  const auto front = var.slice({bin_edge_dim, 0});
  const auto back = var.slice({bin_edge_dim, data_shape});
  const auto front_flat = flatten(front, front.dims().labels(), to_dim);
  const auto back_flat = flatten(back, back.dims().labels(), to_dim);
  if (front_flat.slice({to_dim, 1, front.dims().volume()}) !=
      back_flat.slice({to_dim, 0, back.dims().volume() - 1}))
    throw except::BinEdgeError(
        "Flatten: the bin edges cannot be joined together.");

  // Make the bulk slice of the coord, leaving out the last bin edge
  const auto bulk =
      flatten(var.slice({bin_edge_dim, 0, data_shape}), from_labels, to_dim);
  auto out_dims = bulk.dims();
  // To make the container of the right size, we increase to_dim by 1
  out_dims.resize(to_dim, out_dims[to_dim] + 1);
  auto out = empty(out_dims, var.unit(), var.dtype(), var.hasVariances());
  copy(bulk, out.slice({to_dim, 0, out_dims[to_dim] - 1}));
  copy(back_flat.slice({to_dim, back.dims().volume() - 1}),
       out.slice({to_dim, out_dims[to_dim] - 1}));
  return out;
}

/// Check if one of the from_labels is a bin edge
Dim bin_edge_in_from_labels(const Variable &var, const Dimensions &array_dims,
                            const scipp::span<const Dim> &from_labels) {
  for (const auto &dim : from_labels)
    if (is_edges(array_dims, var.dims(), dim))
      return dim;
  return Dim::Invalid;
}

} // end anonymous namespace

/// Fold a single dimension into multiple dimensions
/// ['x': 6] -> ['y': 2, 'z': 3]
DataArray fold(const DataArray &a, const Dim from_dim,
               const Dimensions &to_dims) {
  return dataset::transform(a, [&](const auto &var) {
    if (is_edges(a.dims(), var.dims(), from_dim))
      return fold_bin_edge(var, from_dim, to_dims);
    else if (var.dims().contains(from_dim))
      return fold(var, from_dim, to_dims);
    else
      return var;
  });
}

/// Flatten multiple dimensions into a single dimension:
/// ['y', 'z'] -> ['x']
DataArray flatten(const DataArray &a, const scipp::span<const Dim> &from_labels,
                  const Dim to_dim) {
  return dataset::transform(a, [&](const auto &in) {
    const auto var =
        (&in == &a.data()) ? in : maybe_broadcast(in, from_labels, a.dims());
    const auto bin_edge_dim =
        bin_edge_in_from_labels(in, a.dims(), from_labels);
    if (bin_edge_dim != Dim::Invalid) {
      return flatten_bin_edge(var, from_labels, to_dim, bin_edge_dim);
    } else if (var.dims().contains(from_labels.front())) {
      return flatten(var, from_labels, to_dim);
    } else {
      return var;
    }
  });
}

DataArray transpose(const DataArray &a, const std::vector<Dim> &dims) {
  return {transpose(a.data(), dims), a.coords(), a.masks(), a.attrs(),
          a.name()};
}

Dataset transpose(const Dataset &d, const std::vector<Dim> &dims) {
  return apply_to_items(
      d, [](auto &&... _) { return transpose(_...); }, dims);
}

} // namespace scipp::dataset
