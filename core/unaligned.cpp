// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <set>

#include "scipp/core/unaligned.h"

namespace scipp::core::unaligned {

namespace {
template <class Map, class Dims>
auto align(DataArray &view, const Dims &unalignedDims) {
  std::vector<std::pair<typename Map::key_type, Variable>> aligned;
  std::set<typename Map::key_type> to_align;
  constexpr auto map = [](DataArray &v) {
    if constexpr (std::is_same_v<Map, CoordsView>)
      return v.coords();
    else if constexpr (std::is_same_v<Map, MasksView>)
      return v.masks();
    else
      return v.attrs();
  };
  for (const auto &[name, item] : map(view)) {
    const auto &dims = item.dims();
    if (std::none_of(unalignedDims.begin(), unalignedDims.end(),
                     [&dims](const Dim dim) { return dims.contains(dim); }) &&
        !(is_events(item) && unalignedDims.count(Dim::Invalid)))
      to_align.insert(name);
  }
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
  if (!unaligned.hasData())
    unaligned.drop_alignment();
  std::set<Dim> binnedDims;
  std::set<Dim> unalignedDims;
  for (const auto &[dim, coord] : coords) {
    expect::equals(coord.unit(), unaligned.coords()[dim].unit());
    binnedDims.insert(dim);
  }
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
  auto alignedCoords = align<CoordsView>(unaligned, unalignedDims);
  const auto dims = unaligned.dims();
  for (const auto &dim : dims.labels()) {
    if (unalignedDims.count(dim)) {
      for (const auto &[d, coord] : coords)
        alignedDims.addInner(d, coord.dims()[d] - 1);
      alignedCoords.insert(alignedCoords.end(), coords.begin(), coords.end());
    } else {
      alignedDims.addInner(dim, unaligned.dims()[dim]);
    }
  }
  if (unalignedDims.count(Dim::Invalid)) {
    for (const auto &[d, coord] : coords)
      alignedDims.addInner(d, coord.dims()[d] - 1);
    alignedCoords.insert(alignedCoords.end(), coords.begin(), coords.end());
  }

  auto name = unaligned.name();
  auto alignedMasks = align<MasksView>(unaligned, unalignedDims);
  auto alignedAttrs = align<AttrsView>(unaligned, unalignedDims);
  return DataArray(UnalignedData{alignedDims, std::move(unaligned)},
                   std::move(alignedCoords), std::move(alignedMasks),
                   std::move(alignedAttrs), std::move(name));
}

Dataset realign(Dataset dataset,
                std::vector<std::pair<Dim, Variable>> newCoords) {
  Dataset out;
  for (const auto &item : dataset)
    out.setData(item.name(), realign(dataset.extract(item.name()), newCoords));
  return out;
}

bool is_realigned_events(const DataArrayConstView &realigned) {
  return !is_events(realigned) && realigned.unaligned() &&
         is_events(realigned.unaligned());
}

VariableConstView realigned_event_coord(const DataArrayConstView &realigned) {
  std::vector<Dim> realigned_dims;
  for (const auto &[dim, coord] : realigned.unaligned().coords())
    if (is_events(coord) && realigned.coords().contains(dim))
      realigned_dims.push_back(dim);
  if (realigned_dims.size() != 1)
    throw except::UnalignedError(
        "Multi-dimensional histogramming of event data not supported yet.");
  return realigned.coords()[realigned_dims.front()];
}

} // namespace scipp::core::unaligned
