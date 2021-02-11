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

bool all_dims_unchanged(const Dimensions &item_dims,
                        const Dimensions &new_dims) {
  for (const auto dim : item_dims.labels()) {
    if (!new_dims.contains(dim))
      return false;
    if (new_dims[dim] != item_dims[dim])
      return false;
  }
  return true;
}

bool contains_all_dim_labels(const Dimensions &item_dims,
                             const Dimensions &old_dims) {
  for (const auto dim : item_dims.labels())
    if (!old_dims.contains(dim))
      return false;
  return true;
}

// Variable reshape_(const VariableConstView &var,
//                   scipp::span<const scipp::index> &new_shape,
//                   scipp::span<const scipp::index> &old_shape) {
//   if (new_shape.size() > old_shape.size())

// }

} // end anonymous namespace

DataArray reshape(const DataArrayConstView &a, const Dimensions &dims) {
  // const auto &old_shape = a.data().shape();
  // constexpr auto reshp = [&dims, &old_shape](const auto &var) {
  //   return reshape_(var, dims.shape(), old_shape);
  // };
  // return transform(a, reshp);

  // ndim_new < ndim_old
  //     : broadcast coords to original data shape and then reshape them all -
  //       ndim_new ==
  //     ndim_old : if shapes are the same,
  //     then keep coords but rename labels,
  //     if not drop coords

  auto reshaped = DataArray(reshape(a.data(), dims));

  if (a.data().dims().labels().size() > dims.labels().size()) {
    for (auto &&[name, coord] : a.coords()) {
      reshaped.coords().set(name,
                            reshape(broadcast(coord, a.data().dims()), dims));
    }
    for (auto &&[name, attr] : a.attrs()) {
      reshaped.attrs().set(name,
                           reshape(broadcast(attr, a.data().dims()), dims));
    }
    for (auto &&[name, mask] : a.masks()) {
      reshaped.masks().set(name,
                           reshape(broadcast(mask, a.data().dims()), dims));
    }
  } else if (a.data().dims().labels().size() == dims.labels().size()) {
  }

  // // build a map

  // for (auto &&[name, coord] : a.coords()) {
  //   auto shp = std::partial_sum(v.begin(), v.end(), v.begin(),
  //                               std::multiplies<int>());
  //   const auto product = std::accumulate(coord.shape().rbegin(), v.end(), 1,
  //                                        std::multiplies<scipp::index>());
  // for (const auto &vol : pro

  // for (auto it = coord.shape().rbegin(); it != my_vector.rend(); ++it) {
  // }

  // if (all_dims_unchanged(coord.dims(), dims))
  //   reshaped.coords().set(name, coord);
  // else if (coord.dims() == a.data().dims())
  //   reshaped.coords().set(name, reshape(coord, dims));
  // }

  // for (auto &&[name, attr] : a.attrs()) {
  //   if (all_dims_unchanged(attr.dims(), dims))
  //     reshaped.attrs().set(name, attr);
  //   else if (attr.dims() == a.data().dims())
  //     reshaped.attrs().set(name, reshape(attr, dims));
  // }
  // for (auto &&[name, mask] : a.masks()) {
  //   if (all_dims_unchanged(mask.dims(), dims))
  //     reshaped.masks().set(name, mask);
  //   else if (mask.dims() == a.data().dims())
  //     reshaped.masks().set(name, reshape(mask, dims));
  //   else if (contains_all_dim_labels(mask.dims(), a.data().dims()))
  //     reshaped.masks().set(name,
  //                          reshape(broadcast(mask, a.data().dims()), dims));
  // }
  return reshaped;
}

// DataArray reshape(const DataArrayConstView &a, const Dimensions &dims) {
//   // The rules are the following:
//   //  - if a coordinate, attribute or mask has all its dimensions unchanged
//   by
//   //    the reshape operation, just copy over to the new DataArray.
//   //  - if a coordinate, attribute or mask has the same dimensions as the
//   data,
//   //    reshape that coordinate, attribute or mask.
//   //  - if a mask has all its dimensions contained in the data dimensions,
//   we
//   //    first broadcast the mask to the data dimensions before reshaping
//   it.
//   //  - if a coordinate, attribute or mask satisfies none of these
//   requirements,
//   //    it is dropped during the reshape operation.
//   auto reshaped = DataArray(reshape(a.data(), dims));
//   for (auto &&[name, coord] : a.coords()) {
//     if (all_dims_unchanged(coord.dims(), dims))
//       reshaped.coords().set(name, coord);
//     else if (coord.dims() == a.data().dims())
//       reshaped.coords().set(name, reshape(coord, dims));
//   }
//   for (auto &&[name, attr] : a.attrs()) {
//     if (all_dims_unchanged(attr.dims(), dims))
//       reshaped.attrs().set(name, attr);
//     else if (attr.dims() == a.data().dims())
//       reshaped.attrs().set(name, reshape(attr, dims));
//   }
//   for (auto &&[name, mask] : a.masks()) {
//     if (all_dims_unchanged(mask.dims(), dims))
//       reshaped.masks().set(name, mask);
//     else if (mask.dims() == a.data().dims())
//       reshaped.masks().set(name, reshape(mask, dims));
//     else if (contains_all_dim_labels(mask.dims(), a.data().dims()))
//       reshaped.masks().set(name,
//                            reshape(broadcast(mask, a.data().dims()),
//                            dims));
//   }
//   return reshaped;
// }

// Dataset reshape(const DatasetConstView &d, const Dimensions &dims) {
//   // Note that we are paying for the coordinate reshaping multiple times
//   here.
//   // It is a trade-off between code simplicity and performance.
//   return apply_to_items(
//       d, [](auto &&... _) { return reshape(_...); }, dims);
// }

} // namespace scipp::dataset
