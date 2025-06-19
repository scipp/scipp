// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/core/except.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/shape.h"

#include "scipp/dataset/except.h"
#include "scipp/dataset/shape.h"

#include "dataset_operations_common.h"

using namespace scipp::variable;

namespace scipp::dataset {

/// Map `op` over `items`, return vector of results
template <class T, class Op> auto map(const T &items, Op op) {
  std::vector<std::decay_t<decltype(op(items.front()))>> out;
  out.reserve(items.size());
  for (const auto &i : items)
    out.emplace_back(op(i));
  return out;
}

/// Concatenate a and b, assuming that a and b contain bin edges.
///
/// Checks that the last edges in `a` match the first edges in `b`. The
/// Concatenates the input edges, removing duplicate bin edges.
Variable join_edges(const std::span<const Variable> vars, const Dim dim) {
  std::vector<Variable> tmp;
  tmp.reserve(vars.size());
  for (const auto &var : vars) {
    if (tmp.empty()) {
      tmp.emplace_back(var);
    } else {
      core::expect::equals(tmp.back().slice({dim, tmp.back().dims()[dim] - 1}),
                           var.slice({dim, 0}));
      tmp.emplace_back(var.slice({dim, 1, var.dims()[dim]}));
    }
  }
  return concat(tmp, dim);
}

namespace {
template <class T, class Key>
bool equal_is_edges(const T &maps, const Key &key, const Dim dim) {
  return std::adjacent_find(maps.begin(), maps.end(),
                            [&key, dim](auto &a, auto &b) {
                              return is_edges(a.sizes(), a[key].dims(), dim) !=
                                     is_edges(b.sizes(), b[key].dims(), dim);
                            }) == maps.end();
}
template <class T, class Key>
bool all_is_edges(const T &maps, const Key &key, const Dim dim) {
  return std::all_of(maps.begin(), maps.end(), [&key, dim](auto &var) {
    return is_edges(var.sizes(), var[key].dims(), dim);
  });
}

template <class T, class Key>
auto broadcast_along_dim(const T &maps, const Key &key, const Dim dim) {
  return map(maps, [&key, dim](const auto &map) {
    const auto &var = map[key];
    return broadcast(var, merge(Dimensions(dim, map.sizes().contains(dim)
                                                    ? map.sizes().at(dim)
                                                    : 1),
                                var.dims()));
  });
}

template <class Maps> auto concat_maps(const Maps &maps, const Dim dim) {
  if (maps.empty())
    throw std::invalid_argument("Cannot concat empty list.");
  using T = typename Maps::value_type;
  core::Dict<typename T::key_type, typename T::mapped_type> out;
  const auto &a = maps.front();
  for (const auto &[key, a_] : a) {
    auto vars =
        map(maps, [&key_ref = key](auto &&map) { return map[key_ref]; });
    if (a.dim_of(key) == dim) {
      if (!equal_is_edges(maps, key, dim)) {
        throw except::BinEdgeError(
            "Either all or none of the inputs must have bin edge coordinates.");
      } else if (!all_is_edges(maps, key, dim)) {
        out.insert_or_assign(key, concat(vars, dim));
      } else {
        out.insert_or_assign(key, join_edges(vars, dim));
      }
    } else {
      // 1D coord is kept only if all inputs have matching 1D coords.
      if (std::any_of(vars.begin(), vars.end(), [dim, &vars](auto &var) {
            return var.dims().contains(dim) || !equals_nan(var, vars.front());
          })) {
        // Mismatching 1D coords must be broadcast to ensure new coord shape
        // matches new data shape.
        out.insert_or_assign(key,
                             concat(broadcast_along_dim(maps, key, dim), dim));
      } else {
        if constexpr (std::is_same_v<T, Masks>)
          out.insert_or_assign(key, copy(a_));
        else
          out.insert_or_assign(key, a_);
      }
    }
  }
  return out;
}

} // namespace

DataArray concat(const std::span<const DataArray> das, const Dim dim) {
  auto out = DataArray(concat(map(das, get_data), dim), {},
                       concat_maps(map(das, get_masks), dim));
  const auto &coords = map(das, get_coords);
  for (auto &&[d, coord] : concat_maps(coords, dim)) {
    coord.set_aligned(d == dim || std::any_of(coords.begin(), coords.end(),
                                              [&d_ref = d](auto &_) {
                                                return _.contains(d_ref) &&
                                                       _[d_ref].is_aligned();
                                              }));
    out.coords().set(d, std::move(coord));
  }
  return out;
}

Dataset concat(const std::span<const Dataset> dss, const Dim dim) {
  if (dss.empty())
    throw std::invalid_argument("Cannot concat empty list.");
  Dataset result;
  for (const auto &first : dss.front())
    if (std::all_of(dss.begin(), dss.end(),
                    [&first](auto &ds) { return ds.contains(first.name()); })) {
      auto das = map(dss, [&first](auto &&ds) { return ds[first.name()]; });
      result.setDataInit(first.name(), concat(das, dim));
    }
  if (result.is_valid())
    return result;
  return Dataset({}, Coords(concat(map(dss, get_sizes), dim),
                            concat_maps(map(dss, get_coords), dim)));
}

DataArray resize(const DataArray &a, const Dim dim, const scipp::index size,
                 const FillValue fill) {
  return apply_to_data_and_drop_dim(
      a, [](auto &&..._) { return resize(_...); }, dim, size, fill);
}

Dataset resize(const Dataset &d, const Dim dim, const scipp::index size,
               const FillValue fill) {
  return apply_to_items(
      d, [](auto &&..._) { return resize(_...); }, dim, size, fill);
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
                         const std::span<const Dim> &from_labels,
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
                          const std::span<const Dim> &from_labels,
                          const Dim to_dim, const Dim bin_edge_dim) {
  const auto data_shape = var.dims()[bin_edge_dim] - 1;

  // Make sure that the bin edges can be joined together
  const auto front = var.slice({bin_edge_dim, 0});
  const auto back = var.slice({bin_edge_dim, data_shape});
  const auto front_flat = flatten(front, front.dims().labels(), to_dim);
  const auto back_flat = flatten(back, back.dims().labels(), to_dim);
  if (front_flat.slice({to_dim, 1, front.dims().volume()}) !=
      back_flat.slice({to_dim, 0, back.dims().volume() - 1}))
    return {};

  // Make the bulk slice of the coord, leaving out the last bin edge
  const auto bulk =
      flatten(var.slice({bin_edge_dim, 0, data_shape}), from_labels, to_dim);
  auto out_dims = bulk.dims();
  // To make the container of the right size, we increase to_dim by 1
  out_dims.resize(to_dim, out_dims[to_dim] + 1);
  auto out = empty(out_dims, var.unit(), var.dtype(), var.has_variances());
  copy(bulk, out.slice({to_dim, 0, out_dims[to_dim] - 1}));
  copy(back_flat.slice({to_dim, back.dims().volume() - 1}),
       out.slice({to_dim, out_dims[to_dim] - 1}));
  return out;
}

/// Check if one of the from_labels is a bin edge
Dim bin_edge_in_from_labels(const Variable &var, const Dimensions &array_dims,
                            const std::span<const Dim> &from_labels) {
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

namespace {
void expect_dimension_subset(const core::Dimensions &full_set,
                             const std::span<const Dim> &subset) {
  for (const auto &dim : subset) {
    if (!full_set.contains(dim)) {
      throw except::DimensionError{"Expected dimension " + to_string(dim) +
                                   "to be in " + to_string(full_set)};
    }
  }
}
} // namespace

/// Flatten multiple dimensions into a single dimension:
/// ['y', 'z'] -> ['x']
DataArray flatten(const DataArray &a,
                  const std::optional<std::span<const Dim>> &from_labels,
                  const Dim to_dim) {
  const auto &labels = from_labels.value_or(a.dims().labels());
  if (from_labels.has_value() && labels.empty())
    return DataArray(flatten(a.data(), labels, to_dim), a.coords(), a.masks());
  expect_dimension_subset(a.dims(), labels);
  return dataset::transform(a, [&](const auto &in) {
    auto var = (&in == &a.data()) ? in : maybe_broadcast(in, labels, a.dims());
    const auto bin_edge_dim = bin_edge_in_from_labels(in, a.dims(), labels);
    if (bin_edge_dim != Dim::Invalid) {
      return flatten_bin_edge(var, labels, to_dim, bin_edge_dim);
    } else if (a.dims().empty() || var.dims().contains(labels.front())) {
      // maybe_broadcast ensures that all variables contain
      // all dims in labels, so only need to check labels.front().
      return flatten(var, labels, to_dim);
    } else {
      // This only happens for metadata.
      return var;
    }
  });
}

DataArray transpose(const DataArray &a, const std::span<const Dim> dims) {
  return {transpose(a.data(), dims), a.coords(), a.masks(), a.name()};
}

Dataset transpose(const Dataset &d, const std::span<const Dim> dims) {
  return apply_to_items(d, [](auto &&..._) { return transpose(_...); }, dims);
}

namespace {
template <class T>
T squeeze_impl(const T &x, const std::optional<std::span<const Dim>> dims) {
  auto squeezed = x;
  for (const auto &dim : dims_for_squeezing(x.dims(), dims)) {
    squeezed = squeezed.slice({dim, 0});
  }
  // Copy explicitly to make sure the output does not have its read-only flag
  // set.
  return T(squeezed);
}
} // namespace
DataArray squeeze(const DataArray &a,
                  const std::optional<std::span<const Dim>> dims) {
  return squeeze_impl(a, dims);
}

Dataset squeeze(const Dataset &d,
                const std::optional<std::span<const Dim>> dims) {
  return squeeze_impl(d, dims);
}

} // namespace scipp::dataset
