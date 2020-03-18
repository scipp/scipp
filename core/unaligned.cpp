// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <set>

#include "scipp/core/unaligned.h"

namespace scipp::core::unaligned {

namespace {
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

Dim unaligned_dim(const VariableConstView &unaligned) {
  if (is_events(unaligned))
    return Dim::Invalid;
  if (unaligned.dims().ndim() != 1)
    throw except::UnalignedError("Coordinate used for alignment must be 1-D.");
  return unaligned.dims().inner();
}
} // namespace

DataArray realign(DataArray unaligned,
                  std::vector<std::pair<Dim, Variable>> coords) {
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
  Dimensions alignedDims;
  std::vector<std::pair<Dim, Variable>> alignedCoords;
  const auto dims = unaligned.dims();
  for (const auto &dim : dims.labels()) {
    if (unalignedDims.count(dim)) {
      for (const auto &[d, coord] : coords)
        alignedDims.addInner(d, coord.dims()[d] - 1);
      alignedCoords.insert(alignedCoords.end(), coords.begin(), coords.end());
    } else {
      alignedDims.addInner(dim, unaligned.dims()[dim]);
      alignedCoords.emplace_back(dim, Variable(unaligned.coords()[dim]));
      unaligned.coords().erase(dim);
    }
  }

  auto name = unaligned.name();
  auto alignedMasks = align<MasksView>(unaligned, unalignedDims);
  auto alignedAttrs = align<AttrsView>(unaligned, unalignedDims);
  return DataArray(UnalignedData{alignedDims, std::move(unaligned)},
                   std::move(alignedCoords), std::move(alignedMasks),
                   std::move(alignedAttrs), std::move(name));
}

} // namespace scipp::core::unaligned
