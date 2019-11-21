// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/histogram.h"
#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/transform.h"

#include "dataset_operations_common.h"

namespace scipp::core {

// For now this implementation is only for the simplest case of 2 dims (inner
// stands for sparse)
DataArray histogram(const DataConstProxy &sparse,
                    const VariableConstProxy &binEdges) {
  if (sparse.hasData())
    throw except::SparseDataError(
        "`histogram` is not implemented for sparse data with values yet.");
  if (sparse.dims().ndim() > 1)
    throw std::logic_error("Only the simple case histograms may be constructed "
                           "for now: 2 dims including sparse.");
  auto dim = binEdges.dims().inner();
  if (binEdges.unit() != sparse.coords()[dim].unit())
    throw std::logic_error(
        "Bin edges must have same unit as the sparse input coordinate.");
  if (binEdges.dtype() != dtype<double> ||
      sparse.coords()[dim].dtype() != DType::Double)
    throw std::logic_error("Histogram is only available for double type.");

  auto result = apply_and_drop_dim(
      sparse,
      [](const DataConstProxy &sparse_, const Dim dim_,
         const VariableConstProxy &binEdges_) {
        const auto &coord = sparse_.coords()[dim_].sparseValues<double>();
        auto edgesSpan = binEdges_.values<double>();
        expect::histogram::sorted_edges(edgesSpan);
        // Copy to avoid slow iterator obtained from VariableConstProxy.
        const std::vector<double> edges(edgesSpan.begin(), edgesSpan.end());
        auto resDims{sparse_.dims()};
        auto len = binEdges_.dims()[dim_] - 1;
        resDims.resize(resDims.index(dim_), len);
        Variable res =
            makeVariableWithVariances<double>(resDims, units::counts);
        if (scipp::numeric::is_linspace(edges)) {
          // Special implementation for linear bins. Gives a 1x to 20x speedup
          // for few and many events per histogram, respectively.
          const double offset = edges.front();
          const double nbin = static_cast<double>(len);
          const double scale = nbin / (edges.back() - edges.front());
          for (scipp::index i = 0; i < sparse_.dims().volume(); ++i) {
            auto curRes = res.values<double>().begin() + i * len;
            for (const auto &c : coord[i]) {
              const double bin = (c - offset) * scale;
              if (bin >= 0.0 && bin < nbin)
                ++(*(curRes + static_cast<scipp::index>(bin)));
            }
          }
        } else {
          for (scipp::index i = 0; i < sparse_.dims().volume(); ++i) {
            auto curRes = res.values<double>().begin() + i * len;
            for (const auto &c : coord[i]) {
              auto it = std::upper_bound(edges.begin(), edges.end(), c);
              if (it != edges.end() && it != edges.begin())
                ++(*(curRes + (--it - edges.begin())));
            }
          }
        }
        std::copy(res.values<double>().begin(), res.values<double>().end(),
                  res.variances<double>().begin());
        return res;
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

bool is_histogram(const DataConstProxy &a, const Dim dim) {
  const auto dims = a.dims();
  const auto coords = a.coords();
  return !dims.sparse() && dims.contains(dim) && coords.contains(dim) &&
         coords[dim].dims().contains(dim) &&
         coords[dim].dims()[dim] == dims[dim] + 1;
}

} // namespace scipp::core
