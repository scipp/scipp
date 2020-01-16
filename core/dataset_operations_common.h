// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DATASET_OPERATIONS_COMMON_H
#define SCIPP_CORE_DATASET_OPERATIONS_COMMON_H

namespace scipp::core {

static inline void validate_coordinates(const VariableConstProxy &var,
                                        const Dim operation_dim) {
  // Get all dimensions of coord
  const auto dims = var.dims();
  // Only if operation dimension is one of these
  if (dims.contains(operation_dim)) {
    auto extent = dims[operation_dim];
    for (scipp::index i = 0; i < extent - 1; ++i) {
      // slice neighbouring increments along operation dimension. Coord
      // slices along these should all be equal.
      const auto layer1 = var.slice({operation_dim, i, i + 1});
      const auto layer2 = var.slice({operation_dim, i + 1, i + 2});
      if (layer1 != layer2) {
        throw except::CoordMismatchError(
            "Coordinates for surviving dimensions do not match");
      }
    }
  }
}


template <bool ApplyToData, class Func, class... Args>
DataArray apply_and_drop_dim_impl(const DataConstProxy &a, Func func,
                                  const Dim dim, Args &&... args) {
  std::map<Dim, Variable> coords;
  for (auto &&[d, coord] : a.coords()) {
    // Coord dimension not same as operation dimension
    if (d != dim) {
      // Coordinates will NOT be dropped and must be the same
      validate_coordinates(coord, dim);
      coords.emplace(d, coord);
    }
  }

  std::map<std::string, Variable> labels;
  for (auto &&[name, label] : a.labels())
    if (!label.dims().contains(dim))
      labels.emplace(name, label);

  std::map<std::string, Variable> attrs;
  for (auto &&[name, attr] : a.attrs())
    if (!attr.dims().contains(dim))
      attrs.emplace(name, attr);

  std::map<std::string, Variable> masks;
  for (auto &&[name, mask] : a.masks())
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
  for (auto &&[d, coord] : a.coords())
    if (coord.dims() != drop)
      coords.emplace(d, coord.dims().contains(dim) ? func(coord, dim)
                                                   : copy(coord));

  std::map<std::string, Variable> labels;
  for (auto &&[name, label] : a.labels())
    if (label.dims() != drop)
      labels.emplace(name, label.dims().contains(dim) ? func(label, dim)
                                                      : copy(label));

  std::map<std::string, Variable> attrs;
  for (auto &&[name, attr] : a.attrs())
    if (attr.dims() != drop)
      attrs.emplace(name,
                    attr.dims().contains(dim) ? func(attr, dim) : copy(attr));

  std::map<std::string, Variable> masks;
  for (auto &&[name, mask] : a.masks())
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
  for (const auto &[name, data] : d)
    result.setData(name, func(data, dim, std::forward<Args>(args)...));
  for (auto &&[name, attr] : d.attrs())
    if (!attr.dims().contains(dim))
      result.setAttr(name, attr);
  return result;
}

} // namespace scipp::core

#endif // SCIPP_CORE_DATASET_OPERATIONS_COMMON_H
