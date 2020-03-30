// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>
#include <set>

#include "scipp/core/event.h"
#include "scipp/core/groupby.h"
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

auto get_bounds(const Dim dim, const VariableConstView &coord) {
  const scipp::index last = coord.dims()[dim] - 1;
  Variable interval(coord.slice({dim, 0, 2}));
  interval.slice({dim, 1}).assign(coord.slice({dim, last}));
  return interval;
}

auto get_bounds(const scipp::span<const std::pair<Dim, Variable>> coords) {
  std::vector<std::pair<Dim, Variable>> bounds;
  for (const auto &[dim, coord] : coords)
    bounds.emplace_back(dim, get_bounds(dim, coord));
  return bounds;
}

/// Return vector of bounds that are tighter than existing ones
auto get_tighter_bounds(const DataArray &array,
                        const std::vector<std::pair<Dim, Variable>> &coords) {
  auto bounds = get_bounds(coords);
  const auto looser_interval = [&array](const auto &item) {
    const auto &[dim, interval] = item;
    if (!array.coords().contains(dim))
      return false;
    const auto old_interval = get_bounds(dim, array.coords()[dim]);
    const auto tightest_interval =
        concatenate(max(concatenate(interval.slice({dim, 0}),
                                    old_interval.slice({dim, 0}), dim),
                        dim),
                    min(concatenate(interval.slice({dim, 1}),
                                    old_interval.slice({dim, 1}), dim),
                        dim),
                    dim);
    return tightest_interval == old_interval;
  };
  if (array.hasData()) {
    // no current bounds, all new bounds are stricter
  } else {
    // previous realignment
    bounds.erase(std::remove_if(bounds.begin(), bounds.end(), looser_interval),
                 bounds.end());
  }
  return bounds;
}

} // namespace

DataArray realign(DataArray unaligned,
                  std::vector<std::pair<Dim, Variable>> coords) {
  const auto bounds = get_tighter_bounds(unaligned, coords);
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

  unaligned = filter_recurse(std::move(unaligned), bounds);

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
  while (!dataset.empty()) {
    auto realigned = realign(dataset.extract(*dataset.keys_begin()), newCoords);
    auto name = realigned.name();
    out.setData(std::move(name), std::move(realigned));
  }
  return out;
}

bool is_realigned_events(const DataArrayConstView &realigned) {
  return !is_events(realigned) && realigned.unaligned() &&
         is_events(realigned.unaligned());
}

Dim realigned_event_dim(const DataArrayConstView &realigned) {
  std::vector<Dim> realigned_dims;
  for (const auto &[dim, coord] : realigned.unaligned().coords())
    if (is_events(coord) && realigned.coords().contains(dim))
      realigned_dims.push_back(dim);
  if (realigned_dims.size() != 1)
    throw except::UnalignedError(
        "Multi-dimensional histogramming of event data not supported yet.");
  return realigned_dims.front();
}

VariableConstView realigned_event_coord(const DataArrayConstView &realigned) {
  return realigned.coords()[realigned_event_dim(realigned)];
}

DataArray
filter_recurse(DataArray &&unaligned,
               const scipp::span<const std::pair<Dim, Variable>> bounds) {
  if (bounds.empty())
    return std::move(unaligned);
  else
    // Could in principle do better with in-place filter.
    return filter_recurse(DataArrayConstView(unaligned), bounds,
                          AttrPolicy::Keep);
}

/// Return new data array based on `unaligned` with any content outside `bounds`
/// removed.
DataArray
filter_recurse(const DataArrayConstView &unaligned,
               const scipp::span<const std::pair<Dim, Variable>> bounds,
               const AttrPolicy attrPolicy) {
  if (bounds.empty())
    return copy(unaligned, attrPolicy);
  const auto &[dim, interval] = bounds[0];
  DataArray filtered =
      is_events(unaligned.coords()[dim])
          ? event::filter(unaligned, dim, interval, attrPolicy)
          : groupby(unaligned, dim, interval).copy(0, attrPolicy);
  if (bounds.size() == 1)
    return filtered;
  return filter_recurse(filtered, bounds.subspan(1), attrPolicy);
}

} // namespace scipp::core::unaligned
