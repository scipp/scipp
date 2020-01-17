// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DATASET_OPERATIONS_COMMON_H
#define SCIPP_CORE_DATASET_OPERATIONS_COMMON_H

namespace scipp::core {

static inline void expectAlignedCoord(const Dim coord_dim,
                                      const VariableConstProxy &var,
                                      const Dim operation_dim) {
  // Coordinate is 2D, but the dimension associated with the coordinate is
  // different from that of the operation. Note we do not account for the
  // possibility that the coordinates actually align along the operation
  // dimension.
  if (var.dims().ndim() > 1)
    throw except::CoordMismatchError(
        "VariableConstProxy Coord/Label has more than one dimension "
        "associated with " +
        to_string(coord_dim) +
        " and will not be reduced by the operation dimension " +
        to_string(operation_dim) + " Terminating operation.");
}

template <bool ApplyToData, class Func, class... Args>
DataArray apply_and_drop_dim_impl(const DataConstProxy &a, Func func,
                                  const Dim dim, Args &&... args) {
  std::map<Dim, Variable> coords;
  for (auto && [ d, coord ] : a.coords()) {
    // Check coordinates will NOT be dropped
    if (d != dim) {
      expectAlignedCoord(d, coord, dim);
      coords.emplace(d, coord);
    }
  }

  std::map<std::string, Variable> labels;
  for (auto && [ name, label ] : a.labels()) {
    // Check coordinates will NOT be dropped
    if (label.dims().inner() != dim) {
      expectAlignedCoord(label.dims().inner(), label, dim);
      labels.emplace(name, label);
    }
  }

  std::map<std::string, Variable> attrs;
  for (auto && [ name, attr ] : a.attrs())
    if (!attr.dims().contains(dim))
      attrs.emplace(name, attr);

  std::map<std::string, Variable> masks;
  for (auto && [ name, mask ] : a.masks())
    if (!mask.dims().contains(dim))
      masks.emplace(name, mask);

  if constexpr (ApplyToData)
    return DataArray(func(a.data(), dim, std::forward<Args>(args)...),
                     std::move(coords), std::move(labels), std::move(masks),
                     std::move(attrs), a.name());
  else
    return DataArray(func(a, dim, std::forward<Args>(args)...),
                     std::move(coords), std::move(labels), std::move(masks),
                     std::move(attrs), a.name());
}

/// Create new data array by applying Func to everything depending on dim, copy
/// otherwise.
template <class Func>
DataArray apply_or_copy_dim(const DataConstProxy &a, Func func, const Dim dim) {
  Dimensions drop({dim, a.dims()[dim]});
  std::map<Dim, Variable> coords;
  // Note the `copy` call, ensuring that the return value of the ternary
  // operator can be moved. Without `copy`, the result of `func` is always
  // copied.
  for (auto && [ d, coord ] : a.coords())
    if (coord.dims() != drop)
      coords.emplace(d, coord.dims().contains(dim) ? func(coord, dim)
                                                   : copy(coord));

  std::map<std::string, Variable> labels;
  for (auto && [ name, label ] : a.labels())
    if (label.dims() != drop)
      labels.emplace(name, label.dims().contains(dim) ? func(label, dim)
                                                      : copy(label));

  std::map<std::string, Variable> attrs;
  for (auto && [ name, attr ] : a.attrs())
    if (attr.dims() != drop)
      attrs.emplace(name,
                    attr.dims().contains(dim) ? func(attr, dim) : copy(attr));

  std::map<std::string, Variable> masks;
  for (auto && [ name, mask ] : a.masks())
    if (mask.dims() != drop)
      masks.emplace(name,
                    mask.dims().contains(dim) ? func(mask, dim) : copy(mask));

  return DataArray(a.hasData() ? func(a.data(), dim)
                               : std::optional<Variable>(),
                   std::move(coords), std::move(labels), std::move(masks),
                   std::move(attrs), a.name());
}

template <class Func, class... Args>
DataArray apply_to_data_and_drop_dim(const DataConstProxy &a, Func func,
                                     const Dim dim, Args &&... args) {
  return apply_and_drop_dim_impl<true>(a, func, dim,
                                       std::forward<Args>(args)...);
}

template <class Func, class... Args>
DataArray apply_and_drop_dim(const DataConstProxy &a, Func func, const Dim dim,
                             Args &&... args) {
  return apply_and_drop_dim_impl<false>(a, func, dim,
                                        std::forward<Args>(args)...);
}

template <class Func, class... Args>
DataArray apply_to_items(const DataConstProxy &d, Func func, Args &&... args) {
  return func(d, std::forward<Args>(args)...);
}

template <class Func, class... Args>
Dataset apply_to_items(const DatasetConstProxy &d, Func func, const Dim dim,
                       Args &&... args) {
  Dataset result;
  for (const auto & [ name, data ] : d)
    result.setData(name, func(data, dim, std::forward<Args>(args)...));
  for (auto && [ name, attr ] : d.attrs())
    if (!attr.dims().contains(dim))
      result.setAttr(name, attr);
  return result;
}

} // namespace scipp::core

#endif // SCIPP_CORE_DATASET_OPERATIONS_COMMON_H
