// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_UNALIGNED_H
#define SCIPP_CORE_UNALIGNED_H

#include <set>

#include "scipp/core/dataset.h"

namespace scipp::core::unaligned {

Dim unaligned_dim(const VariableConstView &unaligned);

namespace detail {
template <class Map, class Dims>
auto align(DataArray &view, const Dims &unalignedDims) {
  std::vector<std::pair<std::string, Variable>> aligned;
  std::set<std::string> to_align;
  constexpr auto map = [](DataArray &v) {
    if constexpr (std::is_same_v<Map, MasksView>)
      return v.masks();
    else
      return v.attrs();
  };
  for (const auto &[name, item] : map(view))
    if (std::none_of(
            unalignedDims.begin(), unalignedDims.end(),
            [&item](const Dim dim) { return item.dims().contains(dim); }))
      to_align.insert(name);
  for (const auto &name : to_align) {
    aligned.emplace_back(name, Variable(map(view)[name]));
    map(view).erase(name);
  }
  return aligned;
}
}

template <class CoordMap = std::vector<std::pair<Dim, Variable>>>
DataArray realign(DataArray unaligned, CoordMap coords) {
  std::set<Dim> binnedDims;
  std::set<Dim> unalignedDims;
  for (const auto &item : coords)
    binnedDims.insert(item.first);
  for (const auto &[dim, coord] : unaligned.coords())
    if (binnedDims.count(dim))
      unalignedDims.insert(unaligned_dim(coord));
  if (unalignedDims.size() < 1)
    throw except::UnalignedError("realign requires at least one unaligned "
                                 "dimension.");
  if (unalignedDims.size() > 1)
    throw except::UnalignedError(
        "realign with more than one unaligned dimension not supported yet.");

  // TODO Some things here can be simplified and optimized by adding an
  // `extract` method to MutableView.
  std::vector<std::pair<Dim, Variable>> alignedCoords;
  const auto dims = unaligned.dims();
  for (const auto &dim : dims.labels()) {
    if (unalignedDims.count(dim)) {
      alignedCoords.insert(alignedCoords.end(), coords.begin(), coords.end());
    } else {
      alignedCoords.emplace_back(dim, Variable(unaligned.coords()[dim]));
      unaligned.coords().erase(dim);
    }
  }
  auto name = unaligned.name();
  auto alignedMasks = detail::align<MasksView>(unaligned, unalignedDims);
  auto alignedAttrs = detail::align<AttrsView>(unaligned, unalignedDims);
  return DataArray(Variable{}, std::move(alignedCoords),
                   std::move(alignedMasks), std::move(alignedAttrs),
                   std::move(name), std::move(unaligned));
}

} // namespace scipp::core::unaligned

#endif // SCIPP_CORE_EVENT_H
