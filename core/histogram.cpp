// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/histogram.h"
#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/groupby.h"
#include "scipp/core/transform_subspan.h"

#include "dataset_operations_common.h"

namespace scipp::core {

static constexpr auto make_histogram =
    [](auto &data, const auto &events, const auto &weights, const auto &edges) {
      constexpr auto value = [](const auto &x, const scipp::index i) {
        if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>) {
          static_cast<void>(i);
          return x.value;
        } else
          return x.values[i];
      };
      constexpr auto variance = [](const auto &x, const scipp::index i) {
        if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>) {
          static_cast<void>(i);
          return x.variance;
        } else
          return x.variances[i];
      };

      // Special implementation for linear bins. Gives a 1x to 20x speedup
      // for few and many events per histogram, respectively.
      if (scipp::numeric::is_linspace(edges)) {
        const auto [offset, nbin, scale] = linear_edge_params(edges);
        for (scipp::index i = 0; i < scipp::size(events); ++i) {
          const auto x = events[i];
          const double bin = (x - offset) * scale;
          if (bin >= 0.0 && bin < nbin) {
            const auto b = static_cast<scipp::index>(bin);
            const auto w = value(weights, i);
            const auto e = variance(weights, i);
            data.value[b] += w;
            data.variance[b] += e;
          }
        }
      } else {
        expect::histogram::sorted_edges(edges);
        for (scipp::index i = 0; i < scipp::size(events); ++i) {
          const auto x = events[i];
          auto it = std::upper_bound(edges.begin(), edges.end(), x);
          if (it != edges.end() && it != edges.begin()) {
            const auto b = --it - edges.begin();
            const auto w = value(weights, i);
            const auto e = variance(weights, i);
            data.value[b] += w;
            data.variance[b] += e;
          }
        }
      }
    };

static constexpr auto make_histogram_unit = [](const units::Unit &sparse_unit,
                                               const units::Unit &weights_unit,
                                               const units::Unit &edge_unit) {
  if (sparse_unit != edge_unit)
    throw except::UnitError("Bin edges must have same unit as the sparse "
                            "input coordinate.");
  if (weights_unit != units::counts && weights_unit != units::dimensionless)
    throw except::UnitError("Weights of sparse data must be "
                            "`units::counts` or `units::dimensionless`.");
  return weights_unit;
};

namespace histogram_detail {
template <class Out, class Coord, class Edge>
using args = std::tuple<span<Out>, event_list<Coord>, span<const Edge>>;
}
namespace histogram_weighted_detail {
template <class Out, class Coord, class Weight, class Edge>
using args = std::tuple<span<Out>, event_list<Coord>, Weight, span<const Edge>>;
}

DataArray histogram(const DataArrayConstView &sparse,
                    const VariableConstView &binEdges) {
  auto dim = binEdges.dims().inner();

  auto result = apply_and_drop_dim(
      sparse,
      [](const DataArrayConstView &sparse_, const Dim dim_,
         const VariableConstView &binEdges_) {
        using namespace histogram_weighted_detail;
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
            dim_, binEdges_.dims()[dim_] - 1, sparse_.coords()[dim_],
            sparse_.data(), binEdges_,
            overloaded{make_histogram, make_histogram_unit,
                       transform_flags::expect_variance_arg<0>,
                       transform_flags::expect_no_variance_arg<1>,
                       transform_flags::expect_variance_arg<2>,
                       transform_flags::expect_no_variance_arg<3>});
      },
      dim, binEdges);
  result.setCoord(dim, binEdges);
  return result;
}

DataArray histogram(const DataArrayConstView &sparse,
                    const Variable &binEdges) {
  return histogram(sparse, VariableConstView(binEdges));
}

Dataset histogram(const Dataset &dataset, const VariableConstView &bins) {
  auto out(Dataset(DatasetConstView::makeViewWithEmptyIndexes(dataset)));
  const Dim dim = bins.dims().inner();
  out.setCoord(dim, bins);
  for (const auto &item : dataset) {
    if (is_events(item.coords()[dim]))
      out.setData(item.name(), histogram(item, bins));
  }
  return out;
}

Dataset histogram(const Dataset &dataset, const Dim &dim) {
  auto bins = dataset.coords()[dim];
  if (is_events(bins))
    throw except::BinEdgeError("Expected bin edges, got event data.");
  return histogram(dataset, bins);
}

/// Return true if the data array respresents a histogram for given dim.
bool is_histogram(const DataArrayConstView &a, const Dim dim) {
  const auto dims = a.dims();
  const auto coords = a.coords();
  return dims.contains(dim) && coords.contains(dim) &&
         coords[dim].dims().contains(dim) &&
         coords[dim].dims()[dim] == dims[dim] + 1;
}

auto extract_group(const GroupBy<DataArray> &grouped,
                   const scipp::index group) {
  const auto &slices = grouped.groups()[group];
  scipp::index size = 0;
  const auto &array = grouped.data();
  for (const auto &slice : slices)
    size += slice.end() - slice.begin();
  const Dim dim = array.coords()[grouped.dim()].dims().inner();
  auto out = copy(array.slice({dim, 0, size}));
  // TODO masks
  scipp::index current = 0;
  for (const auto &slice : slices) {
    const auto thickness = slice.end() - slice.begin();
    const Slice out_slice(slice.dim(), current, current + thickness);
    out.data().slice(out_slice).assign(array.data().slice(slice));
    for (const auto &[d, coord] : out.coords())
      if (coord.dims().contains(dim))
        coord.slice(out_slice).assign(array.coords()[d].slice(slice));
    current += thickness;
  }
  out.coords().erase(grouped.dim());
  return out;
}

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
  if (dim_index == realigned.dims().ndim() - 1) {
    const Dim unaligned_dim = unaligned.coords()[dim].dims().inner();
    auto hist1d = groups.sum(unaligned_dim);
    data.assign(hist1d.data());
    return;
  }
  for (scipp::index i = 0; i < size; ++i) {
    auto slice = extract_group(groups, i);
    histogram_md_recurse(data.slice({dim, i}), slice, realigned, dim_index + 1);
  }
}

DataArray histogram(const DataArrayConstView &realigned) {
  if (realigned.hasData())
    throw except::UnalignedError("Expected realigned data, but data appears to "
                                 "be histogrammed already.");
  // TODO Problem: This contains everything, but below we do not slice removed
  // dims (range slices are ok). Should we simply prevent non-range slicing?
  const auto unaligned = realigned.unaligned();
  Variable data(unaligned.data(), realigned.dims());
  histogram_md_recurse(data, unaligned, realigned);
  return DataArray{std::move(data), realigned.coords()};
}

} // namespace scipp::core
