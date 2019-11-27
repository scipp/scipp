// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DATASET_OPERATIONS_COMMON_H
#define SCIPP_CORE_DATASET_OPERATIONS_COMMON_H

namespace scipp::core {

template <bool ApplyToData, class Func, class... Args>
DataArray apply_and_drop_dim_impl(const DataConstProxy &a, Func func,
                                  const Dim dim, Args &&... args) {
  std::map<Dim, Variable> coords;
  for (auto &&[d, coord] : a.coords())
    if (d != dim)
      coords.emplace(d, coord);

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

/// Create new data array by applying Func to everything depending on dim, drop
/// otherwise.
template <class Func, class... Args>
DataArray apply_or_drop_dim(const DataConstProxy &a, Func func, const Dim dim,
                            Args &&... args) {
  std::map<Dim, Variable> coords;
  for (auto &&[d, coord] : a.coords())
    coords.emplace(d, coord.dims().contains(dim) ? func(coord, dim) : coord);

  std::map<std::string, Variable> labels;
  for (auto &&[name, label] : a.labels())
    labels.emplace(name, label.dims().contains(dim) ? func(label, dim) : label);

  std::map<std::string, Variable> attrs;
  for (auto &&[name, attr] : a.attrs())
    attrs.emplace(name, attr.dims().contains(dim) ? func(attr, dim) : attr);

  std::map<std::string, Variable> masks;
  for (auto &&[name, mask] : a.masks())
    masks.emplace(name, mask.dims().contains(dim) ? func(mask, dim) : mask);

  return DataArray(a.hasData()
                       ? func(a.data(), dim, std::forward<Args>(args)...)
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
