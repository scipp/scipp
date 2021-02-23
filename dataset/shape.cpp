// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/shape.h"

#include "scipp/dataset/except.h"
#include "scipp/dataset/shape.h"

#include "../variable/operations_common.h"
#include "dataset_operations_common.h"
#include <iostream>

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

// bool all_dims_unchanged(const Dimensions &item_dims,
//                         const Dimensions &new_dims) {
//   for (const auto dim : item_dims.labels()) {
//     if (!new_dims.contains(dim))
//       return false;
//     if (new_dims[dim] != item_dims[dim])
//       return false;
//   }
//   return true;
// }

// bool contains_all_dim_labels(const Dimensions &item_dims,
//                              const Dimensions &old_dims) {
//   for (const auto dim : item_dims.labels())
//     if (!old_dims.contains(dim))
//       return false;
//   return true;
// }

// Variable reshape_(const VariableConstView &var,
//                   scipp::span<const scipp::index> &new_shape,
//                   scipp::span<const scipp::index> &old_shape) {
//   if (new_shape.size() > old_shape.size())

// }

// Go through the old dims and:
// - if the dim does not equal the dim that is being stacked, copy dim/shape
// - if the dim equals the dim to be stacked, replace by stack of new dims
const Dimensions stack_dims(const Dimensions &old_dims, const Dim from_dim,
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

//
const Dimensions unstack_dims(const Dimensions &old_dims,
                              const Dimensions &from_dims, const Dim to_dim) {
  // Dimensions stacked_dims;
  // for (const auto dim : from_dims)
  //   stacked_dims.addInner(dim, old_dims[dim]);

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

Variable maybe_broadcast(const VariableConstView &var,
                         const Dimensions &from_dims, const Dim to_dim) {
  // 1. If all stacked dims are contained in the variable's dims, no broadcast
  // 2. If at least one (but not all) of the stacked dims is contained in the
  //    variable's dims, broadcast
  // 3. If none of the variables's dimensions are contained, no broadcast

  // TODO: do we need a std::move?

  const auto &var_dims = var.dims();
  // const auto new_dims = unstack_dims(var_dims, from_dims, to_dim);

  bool contains_one_dim = false;
  bool contains_all_dims = true;
  for (const auto dim : var_dims.labels())
    if (from_dims.contains(dim))
      contains_one_dim = true;
    else
      contains_all_dims = false;
  std::cout << "maybe_broadcast 1 " << contains_one_dim << " "
            << contains_one_dim << std::endl;
  std::cout << var_dims << " " << from_dims << std::endl;

  // if (need_broadcast && !var_dims.contains(from_dims)) {
  if (contains_one_dim && !contains_all_dims) {
    std::cout << "maybe_broadcast 2 " << std::endl;
    Dimensions broadcast_dims;
    for (const auto dim : var_dims.labels())
      if (!from_dims.contains(dim))
        broadcast_dims.addInner(dim, var_dims[dim]);
      else
        for (const auto lab : from_dims.labels())
          // Need to check if the variable contains that dim, and use the
          // variable shape in case we have a bin edge.
          if (var_dims.contains(lab))
            broadcast_dims.addInner(lab, var_dims[lab]);
          else
            broadcast_dims.addInner(lab, from_dims[lab]);
    std::cout << "broadcasting to: " << broadcast_dims << std::endl;
    return broadcast(var, broadcast_dims);
  } else {
    return Variable(std::move(var)); // reshape(v, new_dims);
  }
}

// Variable slice_and_stack(const VariableConstView &v, const Dim from_dim,
//                          const Dimensions &to_dims) {
//   auto view = VariableView(v);
//   for (const auto dim : to_dims.labels()) {
//     view = slice(view, from_dim, 0, )
//   }
// }

/// In the case of bin edges, we perform a series of slices and concatenations
/// to carry out the reshape. This is not the most efficient for large data,
/// but it makes the code simple.
Variable slice_and_stack(const VariableConstView &var, const Dim from_dim,
                         const Dimensions &to_dims) {
  const auto labels = to_dims.labels();

  scipp::index step;
  Variable reshaped;
  auto buf = Variable(var);
  for (int32_t l = 0; l < labels.size() - 1; ++l) {
    step = buf.dims()[from_dim] / to_dims.shape()[l];
    reshaped = Variable(buf.slice({from_dim, 0, step + 1}));
    for (int32_t i = 1; i < to_dims[labels[l]]; ++i)
      reshaped = concatenate(
          reshaped, buf.slice({from_dim, i * step, (i + 1) * step + 1}),
          labels[l]);
    // Copy to update buffer
    buf = reshaped;
  }

  reshaped.rename(from_dim, labels.back());
  return reshaped;
}

Variable slice_and_unstack(const VariableConstView &var,
                           const Dimensions &from_dims, const Dim to_dim) {
  std::cout << "slice_and_unstack 1" << std::endl;
  const auto labels = from_dims.labels();
  // const auto to_lab = labels[labels.size() - 1];
  // scipp::index step;
  Variable reshaped;
  auto buf = Variable(var);
  for (int32_t l = 0; l < labels.size() - 1; ++l) {
    if (buf.dims().contains(labels[l])) {
      // step = buf.dims()[from_dim] / to_dims.shape()[l];
      reshaped = Variable(buf.slice({labels[l], 0}));
      std::cout << "slicing along " << labels[l] << std::endl;
      std::cout << reshaped << std::endl;
      for (int32_t i = 1; i < from_dims.shape()[l]; ++i)
        reshaped = join_edges(VariableView(reshaped), buf.slice({labels[l], i}),
                              labels.back());
      // concatenate(reshaped, buf.slice({labels[l], i}), labels.back());
      // Copy to update buffer
      buf = reshaped;
    }
  }

  buf.rename(labels.back(), to_dim);
  std::cout << "slice_and_unstack 2" << std::endl;
  std::cout << buf << std::endl;
  return buf;
}

const bool has_bin_edges(const VariableConstView &var,
                         const Dimensions &array_dims) {
  // bool has_edges = false;
  for (const auto dim : var.dims().labels())
    if (is_bin_edges(var, array_dims, dim))
      return true;
  return false;
}

} // end anonymous namespace

/// Reshape a single dimension into multiple dimensions:
/// ['x': 6] -> ['y': 2, 'z': 3]
DataArray reshape(const DataArrayConstView &a, const Dim from_dim,
                  const Dimensions &to_dims) {
  // Make sure that new dims do not already exist in data dimensions, apart
  // apart from the old dim (i.e. old dim can be re-used)
  auto old_dims = a.dims();
  for (const auto dim : to_dims.labels())
    if (old_dims.contains(dim) && dim != from_dim)
      throw except::DimensionError(
          "Reshape: new dimensions cannot contain labels that "
          "already exist in the DataArray.");

  auto reshaped =
      DataArray(reshape(a.data(), stack_dims(old_dims, from_dim, to_dims)));

  for (auto &&[name, coord] : a.coords()) {
    if (is_bin_edges(coord, old_dims, from_dim))
      reshaped.coords().set(name, slice_and_stack(coord, from_dim, to_dims));
    else
      reshaped.coords().set(
          name, reshape(coord, stack_dims(coord.dims(), from_dim, to_dims)));
  }

  for (auto &&[name, attr] : a.attrs())
    if (is_bin_edges(attr, old_dims, from_dim))
      reshaped.attrs().set(name, slice_and_stack(attr, from_dim, to_dims));
    else
      reshaped.attrs().set(
          name, reshape(attr, stack_dims(attr.dims(), from_dim, to_dims)));

  // Note that we assume bin-edge masks do not exist
  for (auto &&[name, mask] : a.masks())
    reshaped.masks().set(
        name, reshape(mask, stack_dims(mask.dims(), from_dim, to_dims)));

  return reshaped;
}

/// Reshape multiple dimensions into a single dimension:
/// ['y', 'z'] -> ['x']
DataArray reshape(const DataArrayConstView &a,
                  const std::vector<Dim> &from_dims, const Dim to_dim) {
  auto old_dims = a.dims();
  for (const auto dim : from_dims)
    if (!old_dims.contains(dim))
      throw except::DimensionError(
          "Reshape: dimension to be reshaped not found in DataArray.");

  std::cout << "reshape 1" << std::endl;
  Dimensions stacked_dims;
  for (const auto dim : from_dims)
    stacked_dims.addInner(dim, old_dims[dim]);
  std::cout << "reshape 2" << std::endl;

  auto reshaped = DataArray(
      reshape(a.data(), unstack_dims(old_dims, stacked_dims, to_dim)));
  std::cout << "reshape 3" << std::endl;

  for (auto &&[name, coord] : a.coords()) {
    std::cout << "reshape 4" << std::endl;
    // bool has_bin_edges = false;
    // for (const auto dim : coord.dims().labels()) {
    //   if (is_bin_edges(coord, old_dims, dim)) {
    //     has_bin_edges = true;
    //     break;
    //   }
    // }
    // std::cout << "has_bin_edges: " << has_bin_edges << std::endl;
    if (has_bin_edges(coord, old_dims)) {
      std::cout << "reshape 5" << std::endl;
      // const auto var = maybe_broadcast(coord, stacked_dims, to_dim);
      // try {
      reshaped.coords().set(
          name, slice_and_unstack(maybe_broadcast(coord, stacked_dims, to_dim),
                                  stacked_dims, to_dim));
      std::cout << "reshape 6" << std::endl;
      // } catch (const std::exception&) { /* */ }
    } else {
      std::cout << "reshape 7" << std::endl;
      reshaped.coords().set(
          name, reshape(maybe_broadcast(coord, stacked_dims, to_dim),
                        unstack_dims(coord.dims(), stacked_dims, to_dim)));
      std::cout << "reshape 8" << std::endl;
    }
  }
  std::cout << "reshape 9" << std::endl;

  // for (auto &&[name, attr] : a.attrs())
  //   reshaped.attrs().set(
  //       name, maybe_broadcast_and_reshape(attr, stacked_dims, to_dim));

  // for (auto &&[name, mask] : a.masks())
  //   reshaped.masks().set(
  //       name, maybe_broadcast_and_reshape(mask, stacked_dims, to_dim));

  return reshaped;
}

} // namespace scipp::dataset
