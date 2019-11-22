// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/histogram.h"
#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/transform_subspan.h"

#include "dataset_operations_common.h"

namespace scipp::core {

static constexpr auto make_histogram = [](auto &data, const auto &events,
                                          const auto &edges) {
  if (scipp::numeric::is_linspace(edges)) {
    // Special implementation for linear bins. Gives a 1x to 20x speedup
    // for few and many events per histogram, respectively.
    const auto [offset, nbin, scale] = linear_edge_params(edges);
    for (const auto &e : events) {
      const double bin = (e - offset) * scale;
      if (bin >= 0.0 && bin < nbin)
        ++data.value[static_cast<scipp::index>(bin)];
    }
  } else {
    expect::histogram::sorted_edges(edges);
    for (const auto &e : events) {
      auto it = std::upper_bound(edges.begin(), edges.end(), e);
      if (it != edges.end() && it != edges.begin())
        ++data.value[--it - edges.begin()];
    }
  }
  std::copy(data.value.begin(), data.value.end(), data.variance.begin());
};

static constexpr auto make_histogram_from_weighted =
    [](auto &data, const auto &events, const auto &weights, const auto &edges) {
      if (scipp::numeric::is_linspace(edges)) {
        const auto [offset, nbin, scale] = linear_edge_params(edges);
        for (scipp::index i = 0; i < scipp::size(events); ++i) {
          const auto x = events[i];
          const double bin = (x - offset) * scale;
          if (bin >= 0.0 && bin < nbin) {
            const auto b = static_cast<scipp::index>(bin);
            const auto w = weights.values[i];
            const auto e = weights.variances[i];
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
            const auto w = weights.values[i];
            const auto e = weights.variances[i];
            data.value[b] += w;
            data.variance[b] += e;
          }
        }
      }
    };

static constexpr auto make_histogram_unit = [](const units::Unit &sparse_unit,
                                               const units::Unit &edge_unit) {
  if (sparse_unit != edge_unit)
    throw except::UnitError("Bin edges must have same unit as the sparse "
                            "input coordinate.");
  return units::counts;
};

static constexpr auto make_histogram_unit_from_weighted =
    [](const units::Unit &sparse_unit, const units::Unit &weights_unit,
       const units::Unit &edge_unit) {
      if (sparse_unit != edge_unit)
        throw except::UnitError("Bin edges must have same unit as the sparse "
                                "input coordinate.");
      if (weights_unit != units::counts)
        throw except::UnitError(
            "Weights of sparse data must be `units::counts`.");
      return weights_unit;
    };

namespace histogram_detail {
template <class Out, class Coord, class Edge>
using args = std::tuple<span<Out>, sparse_container<Coord>, span<const Edge>>;
}
namespace histogram_weighted_detail {
template <class Out, class Coord, class Weight, class Edge>
using args = std::tuple<span<Out>, sparse_container<Coord>,
                        sparse_container<Weight>, span<const Edge>>;
}

DataArray histogram(const DataConstProxy &sparse,
                    const VariableConstProxy &binEdges) {
  auto dim = binEdges.dims().inner();

  auto result = apply_and_drop_dim(
      sparse,
      [](const DataConstProxy &sparse_, const Dim dim_,
         const VariableConstProxy &binEdges_) {
        if (sparse_.hasData()) {
          using namespace histogram_weighted_detail;
          return transform_subspan<
              std::tuple<args<double, double, double, double>,
                         args<double, float, double, double>,
                         args<double, float, double, float>>>(
              dim_, binEdges_.dims()[dim_] - 1, sparse_.coords()[dim_],
              sparse_.data(), binEdges_,
              overloaded{make_histogram_from_weighted,
                         make_histogram_unit_from_weighted,
                         transform_flags::expect_variance_arg<0>,
                         transform_flags::expect_no_variance_arg<1>,
                         transform_flags::expect_variance_arg<2>,
                         transform_flags::expect_no_variance_arg<3>});
        } else {
          using namespace histogram_detail;
          return transform_subspan<std::tuple<args<double, double, double>,
                                              args<double, float, double>,
                                              args<double, float, float>>>(
              dim_, binEdges_.dims()[dim_] - 1, sparse_.coords()[dim_],
              binEdges_,
              overloaded{make_histogram, make_histogram_unit,
                         transform_flags::expect_variance_arg<0>,
                         transform_flags::expect_no_variance_arg<1>,
                         transform_flags::expect_no_variance_arg<2>});
        }
      },
      dim, binEdges);
  result.setCoord(dim, binEdges);
  return result;
}

DataArray histogram(const DataConstProxy &sparse, const Variable &binEdges) {
  return histogram(sparse, VariableConstProxy(binEdges));
}

Dataset histogram(const Dataset &dataset, const VariableConstProxy &bins) {
  auto out(Dataset(DatasetConstProxy::makeProxyWithEmptyIndexes(dataset)));
  out.setCoord(bins.dims().inner(), bins);
  for (const auto &[name, item] : dataset) {
    if (item.dims().sparse())
      out.setData(std::string(name), histogram(item, bins));
  }
  return out;
}

Dataset histogram(const Dataset &dataset, const Variable &bins) {
  return histogram(dataset, VariableConstProxy(bins));
}

Dataset histogram(const Dataset &dataset, const Dim &dim) {
  auto bins = dataset.coords()[dim];
  return histogram(dataset, bins);
}

/// Return true if the data array respresents a histogram for given dim.
bool is_histogram(const DataConstProxy &a, const Dim dim) {
  const auto dims = a.dims();
  const auto coords = a.coords();
  return !dims.sparse() && dims.contains(dim) && coords.contains(dim) &&
         coords[dim].dims().contains(dim) &&
         coords[dim].dims()[dim] == dims[dim] + 1;
}

} // namespace scipp::core
