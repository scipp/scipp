// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/histogram.h"
#include "scipp/core/element/histogram.h"
#include "scipp/dataset/bucket.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/groupby.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/transform_subspan.h"

#include "dataset_operations_common.h"

using namespace scipp::core;
using namespace scipp::variable;

namespace scipp::dataset {

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
  if (events.dtype() == dtype<bucket<DataArray>>) {
    // TODO Histogram data from buckets. Is this the natural choice for the API,
    // i.e., does it make sense that histograming considers bucket contents?
    // Should we instead have a separate named function for this case?
    result = apply_and_drop_dim(
        events,
        [](const DataArrayConstView &events_, const Dim dim_,
           const VariableConstView &binEdges_) {
          const auto mask = irreducible_mask(events_.masks(), dim_);
          if (mask)
            // TODO Creating a full copy of event data here is very inefficient
            return buckets::histogram(events_.data() * ~mask, binEdges_);
          else
            return buckets::histogram(events_.data(), binEdges_);
        },
        dim, binEdges);
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

namespace {
template <typename T> bool is_histogram_impl(const T &a, const Dim dim) {
  const auto dims = a.dims();
  const auto coords = a.coords();
  return dims.count(dim) == 1 && coords.contains(dim) &&
         coords[dim].dims().contains(dim) &&
         coords[dim].dims()[dim] == dims.at(dim) + 1;
}
} // namespace
/// Return true if the data array represents a histogram for given dim.
bool is_histogram(const DataArrayConstView &a, const Dim dim) {
  return is_histogram_impl(a, dim);
}

/// Return true if the dataset represents a histogram for given dim.
bool is_histogram(const DatasetConstView &a, const Dim dim) {
  return is_histogram_impl(a, dim);
}

namespace {
/*
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
*/
} // namespace

DataArray histogram(const DataArrayConstView &) {
  throw except::UnalignedError("Expected realigned data, but data appears to "
                               "be histogrammed already.");
  /*
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
    for (const auto &[dim, coord] : realigned.unaligned_coords())
      out.unaligned_coords().set(dim, std::move(coord));
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
  return DataArray{std::move(data), realigned.aligned_coords(),
                   realigned.masks(), realigned.unaligned_coords()};
                   */
}

Dataset histogram(const DatasetConstView &realigned) {
  return apply_to_items(realigned, [](auto &item) { return histogram(item); });
}

} // namespace scipp::dataset
