// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/core/element/histogram.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/histogram.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/transform_subspan.h"

#include "bins_util.h"
#include "dataset_operations_common.h"

using namespace scipp::core;
using namespace scipp::variable;

namespace scipp::dataset {

namespace {
/// Return `var` unchanged if stride along `dim` is 1, else move `dim` to inner
/// dim and return copy such that stride is 1.
auto as_contiguous(const Variable &var, const Dim dim) {
  if (var.strides()[var.dims().index(dim)] == 1)
    return var;
  std::vector<Dim> dims(var.dims().begin(), var.dims().end());
  const auto it = std::find(dims.begin(), dims.end(), dim);
  std::rotate(it, it + 1, dims.end());
  return copy(transpose(var, dims));
}
} // namespace

DataArray histogram(const DataArray &events, const Variable &binEdges) {
  using namespace scipp::core;
  auto dim = binEdges.dims().inner();

  DataArray result;
  if (events.dtype() == dtype<bucket<DataArray>>) {
    // TODO Histogram data from buckets. Is this the natural choice for the API,
    // i.e., does it make sense that histograming considers bucket contents?
    // Should we instead have a separate named function for this case?
    result = apply_and_drop_dim(
        events,
        [](const DataArray &events_, const Dim dim_,
           const Variable &binEdges_) {
          return buckets::histogram(
              hide_masked(events_.data(), events_.masks(),
                          scipp::span<const Dim>{&dim_, 1}),
              binEdges_);
        },
        dim, binEdges);
  } else if (!is_histogram(events, dim)) {
    const auto event_dim = events.coords().dim_of(dim);
    result = apply_and_drop_dim(
        events,
        [dim](const DataArray &events_, const Dim event_dim_,
              const Variable &binEdges_) {
          const auto data = masked_data(events_, event_dim_);
          // Warning: Don't try to move the `as_contiguous` into `subspan_view`
          // without special care: It may return a new variable which will go
          // out of scope, leading to subtle bugs. Here on the other hand the
          // returned temporary is kept alive until the end of the
          // full-expression.
          return transform_subspan(
              events_.dtype(), dim, binEdges_.dims()[dim] - 1,
              subspan_view(as_contiguous(events_.coords()[dim], event_dim_),
                           event_dim_),
              subspan_view(as_contiguous(data, event_dim_), event_dim_),
              binEdges_, element::histogram, "histogram");
        },
        event_dim, binEdges);
  } else {
    throw except::BinEdgeError(
        "Data is already histogrammed. Expected event data or dense point "
        "data, got data with bin edges.");
  }
  result.coords().set(dim, binEdges);
  return result;
}

Dataset histogram(const Dataset &dataset, const Variable &binEdges) {
  return apply_to_items(
      dataset,
      [](const auto &item, const Dim, const auto &binEdges_) {
        return histogram(item, binEdges_);
      },
      binEdges.dims().inner(), binEdges);
}

/// Return the dimensions of the given data array that have an "bin edge"
/// coordinate.
std::set<Dim> edge_dimensions(const DataArray &a) {
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
Dim edge_dimension(const DataArray &a) {
  const auto &dims = edge_dimensions(a);
  if (dims.size() != 1)
    throw except::BinEdgeError("Expected bin edges in only one dimension.");
  return *dims.begin();
}

namespace {
template <typename T> bool is_histogram_impl(const T &a, const Dim dim) {
  const auto dims = a.dims();
  const auto coords = a.coords();
  return dims.contains(dim) && coords.contains(dim) &&
         coords[dim].dims().contains(dim) &&
         coords[dim].dims()[dim] == dims.at(dim) + 1;
}
} // namespace
/// Return true if the data array represents a histogram for given dim.
bool is_histogram(const DataArray &a, const Dim dim) {
  return is_histogram_impl(a, dim);
}

/// Return true if the dataset represents a histogram for given dim.
bool is_histogram(const Dataset &a, const Dim dim) {
  return is_histogram_impl(a, dim);
}

} // namespace scipp::dataset
