// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_UNALIGNED_H
#define SCIPP_CORE_UNALIGNED_H

#include <set>

#include "scipp/core/dataset.h"

namespace scipp::core::unaligned {

Dim unaligned_dim(const VariableConstView &unaligned) {
  if (is_events(unaligned))
    return Dim::Invalid;
  if (unaligned.dims().ndim() != 1)
    throw except::UnalignedError("Coordinate used for alignment must be 1-D.");
  return unaligned.dims().inner();
}

template <class CoordMap = std::vector<std::pair<Dim, Variable>>>
DataArray align(DataArray unaligned, CoordMap coords) {
  std::set<Dim> alignedDims;
  std::set<Dim> binnedDims;
  std::set<Dim> unalignedDims;
  for (const auto &item : coords)
    binnedDims.insert(item.first);
  for (const auto &[dim, coord] : unaligned.coords())
    if (binnedDims.count(dim))
      unalignedDims.insert(unaligned_dim(coord));

  for (const auto &[dim, coord] : unaligned.coords()) {
    bool aligned = true;
    if (unalignedDims.count(Dim::Invalid) && is_events(coord))
      aligned = false;
    else {
      for (const auto unalignedDim : unalignedDims)
        if (coord.dims().contains(unalignedDim))
          aligned = false;
    }
    if (aligned)
      alignedDims.insert(dim);
  }
  // TODO Some things here can be simplified and optimized by adding an
  // `extract` method to MutableView.
  auto name = unaligned.name();
  std::vector<std::pair<Dim, Variable>> alignedCoords;
  for (const auto dim : alignedDims) {
    alignedCoords.emplace_back(dim, Variable(unaligned.coords()[dim]));
    unaligned.coords().erase(dim);
  }
  alignedCoords.insert(alignedCoords.end(), coords.begin(), coords.end());

  return DataArray(Variable{}, std::move(alignedCoords), {}, {},
                   std::move(name), std::move(unaligned));
}

} // namespace scipp::core::unaligned

#endif // SCIPP_CORE_EVENT_H
