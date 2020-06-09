// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/histogram.h"
#include "scipp/core/element/histogram.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/unaligned.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/transform_subspan.h"

#include "dataset_operations_common.h"

using namespace scipp::core;
using namespace scipp::variable;

namespace scipp::dataset {

namespace histogram_events_detail {
template <class Out, class Coord, class Weight, class Edge>
using args = std::tuple<span<Out>, event_list<Coord>, Weight, span<const Edge>>;
}
namespace histogram_dense_detail {
template <class Out, class Coord, class Weight, class Edge>
using args = std::tuple<span<Out>, span<const Coord>, span<const Weight>,
                        span<const Edge>>;
}

DataArray histogram(const DataArrayConstView &events,
                    const VariableConstView &binEdges) {
  using namespace scipp::core;
  auto dim = binEdges.dims().inner();

  DataArray result;
  if (contains_events(events.coords()[dim])) {
    result = apply_and_drop_dim(
        events,
        [](const DataArrayConstView &events_, const Dim eventDim,
           const Dim dim_, const VariableConstView &binEdges_) {
          static_cast<void>(eventDim); // This is just Dim::Invalid
          using namespace histogram_events_detail;
          // This supports scalar weights as well as event_list weights.
          return transform_subspan<
              std::tuple<args<double, double, double, double>,
                         args<double, float, double, double>,
                         args<double, float, double, float>,
                         args<double, double, float, double>,
                         args<double, double, event_list<double>, double>,
                         args<double, float, event_list<double>, double>,
                         args<double, float, event_list<double>, float>,
                         args<double, double, event_list<float>, double>>>(
              dtype<double>, dim_, binEdges_.dims()[dim_] - 1,
              events_.coords()[dim_], events_.data(), binEdges_,
              element::histogram);
        },
        dim_of_coord(events.coords()[dim], dim), dim, binEdges);
  } else if (!is_histogram(events, dim)) {
    result = apply_and_drop_dim(
        events,
        [](const DataArrayConstView &events_, const Dim dim_,
           const VariableConstView &binEdges_) {
          const auto mask = irreducible_mask(events_.masks(), dim_);
          Variable masked;
          if (mask)
            masked = events_.data() * ~mask;
          using namespace histogram_dense_detail;
          return transform_subspan<
              std::tuple<args<double, double, double, double>,
                         args<float, double, float, double>,
                         args<double, float, double, float>,
                         args<float, float, float, float>>>(
              dtype<double>, dim_, binEdges_.dims()[dim_] - 1,
              events_.coords()[dim_],
              mask ? VariableConstView(masked) : events_.data(), binEdges_,
              element::histogram);
        },
        dim, binEdges);
  } else {
    throw except::BinEdgeError(
        "Data is already histogrammed. Expected event data or dense point "
        "data, got data with bin edges.");
  }
  result.coords().set(dim, binEdges);
  return result;
}

Dataset histogram(const DatasetConstView &dataset,
                  const VariableConstView &binEdges) {
  return apply_to_items(
      dataset,
      [](const auto &item, const Dim, const auto &binEdges_) {
        return histogram(item, binEdges_);
      },
      binEdges.dims().inner(), binEdges);
}

/// Return the dimensions of the given data array that have an "bin edge"
/// coordinate.
std::set<Dim> edge_dimensions(const DataArrayConstView &a) {
  const auto coords = a.coords();
  std::set<Dim> dims;
  for (const auto &[d, coord] : a.coords())
    if (a.dims().contains(d) && coord.dims().contains(d) &&
        coord.dims()[d] == a.dims()[d] + 1)
      dims.insert(d);
  return dims;
}

/// Return the Dim of the given data array that has an "bin edge" coordinate.
///
/// Throws if there is not excactly one such dimension.
Dim edge_dimension(const DataArrayConstView &a) {
  const auto &dims = edge_dimensions(a);
  if (dims.size() != 1)
    throw except::BinEdgeError("Expected bin edges in only one dimension.");
  return *dims.begin();
}

/// Return true if the data array respresents a histogram for given dim.
bool is_histogram(const DataArrayConstView &a, const Dim dim) {
  const auto dims = a.dims();
  const auto coords = a.coords();
  return dims.contains(dim) && coords.contains(dim) &&
         coords[dim].dims().contains(dim) &&
         coords[dim].dims()[dim] == dims[dim] + 1;
}

namespace {
void histogram_md_recurse(const VariableView &data,
                          const DataArrayConstView &unaligned,
                          const DataArrayConstView &realigned,
                          const scipp::index dim_index = 0) {
  const auto &dims = realigned.dims();
  const Dim dim = dims.labels()[dim_index];
  const auto size = dims.shape()[dim_index];
  if (unaligned.dims().contains(dim)) // skip over aligned dims
    return histogram_md_recurse(data, unaligned, realigned, dim_index + 1);
  auto groups = groupby(unaligned, dim, realigned.coords()[dim]);
  if (data.dims().ndim() == unaligned.dims().ndim()) {
    const Dim unaligned_dim = unaligned.coords()[dim].dims().inner();
    auto hist1d = groups.sum(unaligned_dim);
    data.assign(hist1d.data());
    return;
  }
  for (scipp::index i = 0; i < size; ++i) {
    auto slice = groups.copy(i, AttrPolicy::Drop);
    slice.coords().erase(dim); // avoid carry of unnecessary coords in recursion
    histogram_md_recurse(data.slice({dim, i}), slice, realigned, dim_index + 1);
  }
}
} // namespace

DataArray histogram(const DataArrayConstView &realigned) {
  if (realigned.hasData())
    throw except::UnalignedError("Expected realigned data, but data appears to "
                                 "be histogrammed already.");
  if (unaligned::is_realigned_events(realigned)) {
    const auto realigned_dims = unaligned::realigned_event_dims(realigned);
    auto bounds = realigned.slice_bounds();
    bounds.erase(std::remove_if(bounds.begin(), bounds.end(),
                                [&realigned_dims](const auto &item) {
                                  return realigned_dims.count(item.first);
                                }),
                 bounds.end());
    DataArray out;
    if (bounds.empty())
      out = histogram(realigned.unaligned(),
                      unaligned::realigned_event_coord(realigned));
    else {
      // Copy to drop events out of slice bounds
      DataArray sliced(realigned);
      out = histogram(sliced.unaligned(),
                      unaligned::realigned_event_coord(realigned));
    }
    for (const auto &[name, attr] : realigned.attrs())
      out.attrs().set(name, std::move(attr));
    return out;
  }
  std::optional<DataArray> filtered;
  // If `realigned` is sliced we need to copy the unaligned content to "apply"
  // the slicing since slicing realigned dimensions does not affect the view
  // onto the unaligned content. Note that we could in principle avoid the copy
  // if only aligned dimensions are sliced.
  if (!realigned.slices().empty())
    filtered = DataArray(realigned, AttrPolicy::Drop);
  const auto unaligned =
      filtered ? filtered->unaligned() : realigned.unaligned();

  Variable data(unaligned.data(), realigned.dims());
  histogram_md_recurse(data, unaligned, realigned);
  return DataArray{std::move(data), realigned.coords(), realigned.masks(),
                   realigned.attrs()};
}

Dataset histogram(const DatasetConstView &realigned) {
  return apply_to_items(realigned, [](auto &item) { return histogram(item); });
}

} // namespace scipp::dataset
