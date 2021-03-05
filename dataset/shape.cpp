// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
        // Mismatching 1D coords must be broadcast to ensure new coord shape
        // matches new data shape.
        out.emplace(
            key,
            concatenate(
                broadcast(a_, merge(dimsA.count(dim)
                                        ? Dimensions(dim, dimsA.at(dim))
                                        : Dimensions(),
                                    a_.dims())),
                broadcast(b[key], merge(dimsB.count(dim)
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

DataArray concatenate(const DataArrayConstView &a, const DataArrayConstView &b,
                      const Dim dim) {
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

Dataset concatenate(const DatasetConstView &a, const DatasetConstView &b,
                    const Dim dim) {
  // Note that in the special case of a dataset without data items (only coords)
  // concatenating a range slice with a non-range slice will fail due to the
  // missing unaligned coord in the non-range slice. This is an extremely
  // special case and cannot be handled without adding support for unaligned
  // coords to dataset (which is not desirable for a variety of reasons). It is
  // unlikely that this will cause trouble in practice. Users can just use a
  // range slice of thickness 1.
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

DataArray resize(const DataArrayConstView &a, const Dim dim,
                 const DataArrayConstView &shape) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&v, const Dim, auto &&s) { return resize(v, s); }, dim,
      shape.data());
}

Dataset resize(const DatasetConstView &d, const Dim dim,
               const DatasetConstView &shape) {
  Dataset result;
  for (const auto &data : d)
    result.setData(data.name(), resize(data, dim, shape[data.name()]));
  return result;
}

namespace {

/// Split dims for reshaping one dim into multiple dims
///
/// Go through the old dims and:
/// - if the dim does not equal the dim that is being stacked, copy dim/shape
/// - if the dim equals the dim to be stacked, replace by stack of new dims
const Dimensions split_dims(const Dimensions &old_dims, const Dim from_dim,
                            const Dimensions &to_dims) {
  Dimensions new_dims;
  for (const auto dim : old_dims.labels())
    if (dim != from_dim)
      new_dims.addInner(dim, old_dims[dim]);
    else
      for (const auto lab : to_dims.labels())
        new_dims.addInner(lab, to_dims[lab]);
  return new_dims;
}

/// Flatten dims for reshaping multiple dims into one
///
/// Go through the old dims and:
/// - if the dim is contained in the list of dims to be flattened, add the new
///   dim once
/// - if not, copy the dim/shape
const Dimensions flatten_dims(const Dimensions &old_dims,
                              const Dimensions &from_dims, const Dim to_dim) {
  Dimensions new_dims;
  for (const auto dim : old_dims.labels())
    if (from_dims.contains(dim)) {
      if (!new_dims.contains(to_dim))
        new_dims.addInner(to_dim, from_dims.volume());
    } else {
      new_dims.addInner(dim, old_dims[dim]);
    }
  return new_dims;
}

/// Either broadcast variable to from_dims before a reshape or not:
///
/// 1. If all from_dims are contained in the variable's dims, no broadcast
/// 2. If at least one (but not all) of the from_dims is contained in the
///    variable's dims, broadcast
/// 3. If none of the variables's dimensions are contained, no broadcast
Variable maybe_broadcast(const VariableConstView &var,
                         const Dimensions &from_dims) {
  const auto &var_dims = var.dims();

  Dimensions broadcast_dims;
  for (const auto dim : var_dims.labels())
    if (!from_dims.contains(dim))
      broadcast_dims.addInner(dim, var_dims[dim]);
    else
      for (const auto lab : from_dims.labels())
        if (!broadcast_dims.contains(lab)) {
          // Need to check if the variable contains that dim, and use the
          // variable shape in case we have a bin edge.
          if (var_dims.contains(lab))
            broadcast_dims.addInner(lab, var_dims[lab]);
          else
            broadcast_dims.addInner(lab, from_dims[lab]);
        }
  return broadcast(var, broadcast_dims);
}

/// Special handling for splitting coord along a dim that contains bin edges.
///
/// The procedure is the following:
/// - reshape the coord, excluding the last bin-edge slice
/// - slice the first element along the inner dimension of the reshaped var
/// - reshape it so it has the same number of dims as the original coord
/// - slice out the first element along the from_dim
/// - concatenate the last bin-edge slice of the original coord
/// - reshape this into the dimensions of the original edge slice
/// - concatenate this to the reshaped coord that excluded the last bin edge
Variable split_bin_edge(const VariableConstView &var, const Dim from_dim,
                        const Dimensions &to_dims) {
  const auto slice = var.slice({from_dim, 0, var.dims()[from_dim] - 1});
  const auto reshaped =
      reshape(slice, split_dims(slice.dims(), from_dim, to_dims));
  const auto var_dims = var.dims();

  // Slice along the inner dimension of the to dims
  const auto edge = reshaped.slice({to_dims.inner(), 0});
  const auto &edge_dims = edge.dims();

  Variable duplicate_edges;
  // Compare the dimensions of edge and the original coord.
  // We want to flatten any dimension which is not in the original coord.
  if (edge_dims.ndim() > var_dims.ndim()) {
    Dimensions flat_dims;
    scipp::index volume = edge_dims.volume();
    for (const auto dim : edge_dims.labels()) {
      if (!to_dims.contains(dim)) {
        volume /= edge_dims[dim];
        if (!flat_dims.contains(from_dim))
          flat_dims.addInner(from_dim, 1);
      } else {
        flat_dims.addInner(dim, edge_dims[dim]);
      }
    }
    // Update the flat_dims shape now that the volume has been fully
    // decomposed
    flat_dims.resize(from_dim, volume);
    // Now reshape the edge to the flattened dims
    duplicate_edges = reshape(edge, flat_dims);
  } else {
    duplicate_edges = Variable(edge);
  }

  // Now concatenate the edge (minus its first element) and the last slice
  // of coord
  const auto outer_dim = duplicate_edges.dims().labels()[0];
  const auto end_cap = concatenate(
      duplicate_edges.slice({outer_dim, 1, duplicate_edges.dims()[outer_dim]}),
      var.slice({from_dim, var.dims()[from_dim] - 1}), outer_dim);
  // Reshape the end cap to original coord slice dims.
  // This is basically just a quick way of renaming the dims.
  return concatenate(reshaped, reshape(end_cap, edge_dims), to_dims.inner());
}

/// Special handling for flattening coord along a dim that contains bin edges.
///
/// The procedure is the following:
/// - check that the front slice along the bin edge dim matches the back slice,
///   after offseting the values by 1
/// - reshape the variable, excluding the last bin edge
/// - concatenate the very last bin edge to the result
Variable flatten_bin_edge(const VariableConstView &var,
                          const Dimensions &from_dims, const Dim to_dim,
                          const Dim bin_edge_dim) {
  const auto data_shape = var.dims()[bin_edge_dim] - 1;
  // Make sure that the bin edges match
  const auto front = var.slice({bin_edge_dim, 0});
  const auto back = var.slice({bin_edge_dim, data_shape});
  const auto front_flat = reshape(front, {{to_dim, front.dims().volume()}});
  const auto back_flat = reshape(back, {{to_dim, back.dims().volume()}});
  // Check that bin edges can be joined together
  if (front_flat.slice({to_dim, 1, front.dims().volume()}) !=
      back_flat.slice({to_dim, 0, back.dims().volume() - 1}))
    throw except::BinEdgeError(
        "Flatten: the bin edges cannot be joined together.");

  const auto base = var.slice({bin_edge_dim, 0, data_shape});
  return concatenate(
      reshape(base, flatten_dims(base.dims(), from_dims, to_dim)),
      back_flat.slice({to_dim, back.dims().volume() - 1}), to_dim);
}

/// Check if one of the from_dims is a bin edge
Dim bin_edge_in_from_dims(const VariableConstView &var,
                          const Dimensions &array_dims,
                          const Dimensions &from_dims) {
  for (const auto dim : from_dims.labels())
    if (is_bin_edges(var, array_dims, dim))
      return dim;
  return Dim::Invalid;
}

} // end anonymous namespace

/// Split a single dimension into multiple dimensions:
/// ['x': 6] -> ['y': 2, 'z': 3]
DataArray split(const DataArrayConstView &a, const Dim from_dim,
                const Dimensions &to_dims) {
  // Make sure that new dims do not already exist in data dimensions,
  // apart apart from the old dim (i.e. old dim can be re-used)
  const auto &old_dims = a.dims();
  for (const auto dim : to_dims.labels())
    if (old_dims.contains(dim) && dim != from_dim)
      throw except::DimensionError(
          "Split: new dimensions cannot contain labels that "
          "already exist in the DataArray.");

  auto reshaped =
      DataArray(reshape(a.data(), split_dims(old_dims, from_dim, to_dims)));

  for (auto &&[name, coord] : a.coords()) {
    if (is_bin_edges(coord, old_dims, from_dim))
      reshaped.coords().set(name, split_bin_edge(coord, from_dim, to_dims));
    else
      reshaped.coords().set(
          name, reshape(coord, split_dims(coord.dims(), from_dim, to_dims)));
  }

  for (auto &&[name, attr] : a.attrs())
    if (is_bin_edges(attr, old_dims, from_dim))
      reshaped.attrs().set(name, split_bin_edge(attr, from_dim, to_dims));
    else
      reshaped.attrs().set(
          name, reshape(attr, split_dims(attr.dims(), from_dim, to_dims)));

  // Note that we assume bin-edge masks do not exist
  for (auto &&[name, mask] : a.masks())
    reshaped.masks().set(
        name, reshape(mask, split_dims(mask.dims(), from_dim, to_dims)));

  return reshaped;
}

/// Flatten multiple dimensions into a single dimension:
/// ['y', 'z'] -> ['x']
DataArray flatten(const DataArrayConstView &a,
                  const std::vector<Dim> &from_labels, const Dim to_dim) {
  const auto &old_dims = a.dims();
  for (const auto dim : from_labels)
    if (!old_dims.contains(dim))
      throw except::DimensionError(
          "Flatten: dimension to be flattened not found in DataArray.");

  Dimensions from_dims;
  for (const auto dim : from_labels)
    from_dims.addInner(dim, old_dims[dim]);

  // Only allow reshaping contiguous dimensions.
  // Note that isContiguousIn only allows for inner contiguous blocks,
  // and contains(dimensions) ignores dimension order.
  const auto offset = old_dims.index(from_labels[0]);
  for (scipp::index i = 0; i < from_dims.ndim(); ++i) {
    if (old_dims.label(i + offset) != from_dims.label(i) ||
        old_dims.size(i + offset) != from_dims.size(i))
      throw except::DimensionError(
          "Flatten: can only flatten a contiguous set of dimensions. "
          "The order of the dimensions to flatten must also match the order of "
          "dimensions in the DataArray.");
  }

  auto reshaped =
      DataArray(reshape(a.data(), flatten_dims(old_dims, from_dims, to_dim)));

  Dim bin_edge_dim;

  for (auto &&[name, coord] : a.coords()) {
    bin_edge_dim = bin_edge_in_from_dims(coord, old_dims, from_dims);
    if (bin_edge_dim != Dim::Invalid) {
      reshaped.coords().set(name,
                            flatten_bin_edge(maybe_broadcast(coord, from_dims),
                                             from_dims, to_dim, bin_edge_dim));
    } else {
      reshaped.coords().set(
          name, reshape(maybe_broadcast(coord, from_dims),
                        flatten_dims(coord.dims(), from_dims, to_dim)));
    }
  }

  for (auto &&[name, attr] : a.attrs()) {
    bin_edge_dim = bin_edge_in_from_dims(attr, old_dims, from_dims);
    if (bin_edge_dim != Dim::Invalid) {
      reshaped.attrs().set(name,
                           flatten_bin_edge(maybe_broadcast(attr, from_dims),
                                            from_dims, to_dim, bin_edge_dim));

    } else {
      reshaped.attrs().set(
          name, reshape(maybe_broadcast(attr, from_dims),
                        flatten_dims(attr.dims(), from_dims, to_dim)));
    }
  }

  for (auto &&[name, mask] : a.masks())
    reshaped.masks().set(name,
                         reshape(maybe_broadcast(mask, from_dims),
                                 flatten_dims(mask.dims(), from_dims, to_dim)));

  return reshaped;
}

} // namespace scipp::dataset
