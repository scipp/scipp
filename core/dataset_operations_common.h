// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DATASET_OPERATIONS_COMMON_H
#define SCIPP_CORE_DATASET_OPERATIONS_COMMON_H

namespace scipp::core {

/// Return the dimension for given coord or labels.
///
/// For dimension-coords, this is the same as the key, for non-dimension-coords
/// (labels) we adopt the convention that labels are "labelling" their inner
/// dimension.
template <class T, class Key>
Dim dim_of_coord_or_labels(const T &var, const Key &key) {
  if constexpr (std::is_same_v<Key, Dim>) {
    const bool is_dimension_coord = var.dims().contains(key);
    return is_dimension_coord ? key : var.dims().inner();
  } else
    return var.dims().inner();
}

static inline void expectAlignedCoord(const Dim coord_dim,
                                      const VariableConstView &var,
                                      const Dim operation_dim) {
  // Coordinate is 2D, but the dimension associated with the coordinate is
  // different from that of the operation. Note we do not account for the
  // possibility that the coordinates actually align along the operation
  // dimension.
  if (var.dims().ndim() > 1)
    throw except::CoordMismatchError(
        "VariableConstView Coord/Label has more than one dimension "
        "associated with " +
        to_string(coord_dim) +
        " and will not be reduced by the operation dimension " +
        to_string(operation_dim) + " Terminating operation.");
}

template <bool ApplyToData, class Func, class... Args>
DataArray apply_and_drop_dim_impl(const DataArrayConstView &a, Func func,
                                  const Dim dim, Args &&... args) {
  std::map<Dim, Variable> coords;
  for (auto &&[d, coord] : a.coords()) {
    // Check coordinates will NOT be dropped
    if (dim_of_coord_or_labels(coord, d) != dim) {
      expectAlignedCoord(d, coord, dim);
      coords.emplace(d, coord);
    }
  }

  std::map<std::string, Variable> labels;
  for (auto &&[name, label] : a.labels()) {
    // Check coordinates will NOT be dropped
    if (label.dims().inner() != dim) {
      expectAlignedCoord(label.dims().inner(), label, dim);
      labels.emplace(name, label);
    }
  }

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
template <class Func, class... Args>
DataArray apply_or_copy_dim(const DataArrayConstView &a, Func func,
                            const Dim dim, Args &&... args) {
  Dimensions drop({dim, a.dims()[dim]});
  std::map<Dim, Variable> coords;
  // Note the `copy` call, ensuring that the return value of the ternary
  // operator can be moved. Without `copy`, the result of `func` is always
  // copied.
  for (auto &&[d, coord] : a.coords())
    if (coord.dims() != drop)
      coords.emplace(d, coord.dims().contains(dim) ? func(coord, dim, args...)
                                                   : copy(coord));

  std::map<std::string, Variable> labels;
  for (auto &&[name, label] : a.labels())
    if (label.dims() != drop)
      labels.emplace(name, label.dims().contains(dim)
                               ? func(label, dim, args...)
                               : copy(label));

  std::map<std::string, Variable> attrs;
  for (auto &&[name, attr] : a.attrs())
    if (attr.dims() != drop)
      attrs.emplace(name, attr.dims().contains(dim) ? func(attr, dim, args...)
                                                    : copy(attr));

  std::map<std::string, Variable> masks;
  for (auto &&[name, mask] : a.masks())
    if (mask.dims() != drop)
      masks.emplace(name, mask.dims().contains(dim) ? func(mask, dim, args...)
                                                    : copy(mask));

  return DataArray(a.hasData() ? func(a.data(), dim, args...)
                               : std::optional<Variable>(),
                   std::move(coords), std::move(labels), std::move(masks),
                   std::move(attrs), a.name());
}

template <class Func, class... Args>
DataArray apply_to_data_and_drop_dim(const DataArrayConstView &a, Func func,
                                     const Dim dim, Args &&... args) {
  return apply_and_drop_dim_impl<true>(a, func, dim,
                                       std::forward<Args>(args)...);
}

template <class Func, class... Args>
DataArray apply_and_drop_dim(const DataArrayConstView &a, Func func,
                             const Dim dim, Args &&... args) {
  return apply_and_drop_dim_impl<false>(a, func, dim,
                                        std::forward<Args>(args)...);
}

template <class Func, class... Args>
DataArray apply_to_items(const DataArrayConstView &d, Func func,
                         Args &&... args) {
  return func(d, std::forward<Args>(args)...);
}

template <class Func, class... Args>
Dataset apply_to_items(const DatasetConstView &d, Func func, const Dim dim,
                       Args &&... args) {
  Dataset result;
  for (const auto &data : d)
    result.setData(data.name(), func(data, dim, std::forward<Args>(args)...));
  for (auto &&[name, attr] : d.attrs())
    if (!attr.dims().contains(dim))
      result.setAttr(name, attr);
  return result;
}

// Helpers for reductions for DataArray and Dataset, which include masks.
[[nodiscard]] Variable mean(const VariableConstView &var, const Dim dim,
                            const MasksConstView &masks);
VariableView mean(const VariableConstView &var, const Dim dim,
                  const MasksConstView &masks, const VariableView &out);
[[nodiscard]] Variable flatten(const VariableConstView &var, const Dim dim,
                               const MasksConstView &masks);
[[nodiscard]] Variable sum(const VariableConstView &var, const Dim dim,
                           const MasksConstView &masks);
VariableView sum(const VariableConstView &var, const Dim dim,
                 const MasksConstView &masks, const VariableView &out);

} // namespace scipp::core

#endif // SCIPP_CORE_DATASET_OPERATIONS_COMMON_H
