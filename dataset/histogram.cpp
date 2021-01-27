// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/histogram.h"
#include "scipp/core/element/histogram.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/groupby.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/transform_subspan.h"

#include "dataset_operations_common.h"

using namespace scipp::core;
using namespace scipp::variable;

namespace scipp::dataset {

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
          const Masker masker(events_, dim_);
          // TODO Creating a full copy of event data here is very inefficient
          return buckets::histogram(masker.data(), binEdges_);
        },
        dim, binEdges);
  } else if (!is_histogram(events, dim)) {
    const auto data_dim = events.dims().inner();
    result = apply_and_drop_dim(
        events,
        [](const DataArrayConstView &events_, const Dim data_dim_,
           const VariableConstView &binEdges_) {
          const auto dim_ = binEdges_.dims().inner();
          const Masker masker(events_, dim_);
          return transform_subspan(
              events_.dtype(), dim_, binEdges_.dims()[dim_] - 1,
              subspan_view(events_.coords()[dim_], data_dim_),
              subspan_view(masker.data(), data_dim_), binEdges_,
              element::histogram);
        },
        data_dim, binEdges);
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

} // namespace scipp::dataset
