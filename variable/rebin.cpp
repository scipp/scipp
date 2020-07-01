// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock, Igor Gudich
#include "scipp/core/element/rebin.h"
#include "scipp/units/except.h"
#include "scipp/variable/apply.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/except.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/transform_subspan.h"

namespace scipp::variable {

bool isBinEdge(const Dim dim, Dimensions edges, const Dimensions &toMatch) {
  edges.resize(dim, edges[dim] - 1);
  return edges[dim] == toMatch[dim];
}

template <typename T>
void rebin_non_inner(const Dim dim, const VariableConstView &oldT,
                     Variable &newT, const VariableConstView &oldCoordT,
                     const VariableConstView &newCoordT) {
  const auto oldSize = oldT.dims()[dim];
  const auto newSize = newT.dims()[dim];

  const auto *xold = oldCoordT.values<T>().data();
  const auto *xnew = newCoordT.values<T>().data();
  // This function assumes that dimensions between coord and data
  // coord is 1D.
  int iold = 0;
  int inew = 0;
  // TODO: Using dtype<bool> does not seem to compile on Windows. It is not
  // clear why that is, since dtype<double> is working below.
  // We use the longer syntax below which compiles instead of:
  // const bool is_bool = newT.dtype() == dtype<bool>;
  const bool is_bool = newT.dtype().index == std::type_index(typeid(bool));
  while ((iold < oldSize) && (inew < newSize)) {
    auto xo_low = xold[iold];
    auto xo_high = xold[iold + 1];
    auto xn_low = xnew[inew];
    auto xn_high = xnew[inew + 1];

    if (xn_high <= xo_low)
      inew++; /* old and new bins do not overlap */
    else if (xo_high <= xn_low)
      iold++; /* old and new bins do not overlap */
    else {
      if (is_bool) {
        newT.slice({dim, inew}) |= oldT.slice({dim, iold});
      } else {
        // delta is the overlap of the bins on the x axis
        auto delta = std::min(xn_high, xo_high) - std::max(xn_low, xo_low);
        auto owidth = xo_high - xo_low;
        newT.slice({dim, inew}) +=
            astype(oldT.slice({dim, iold}) * ((delta / owidth) * units::one),
                   newT.dtype());
      }
      if (xn_high > xo_high) {
        iold++;
      } else {
        inew++;
      }
    }
  }
}

namespace rebin_inner_detail {
template <class Out, class OutEdge, class In, class InEdge>
using args = std::tuple<span<Out>, span<const OutEdge>, span<const In>,
                        span<const InEdge>>;
}

Variable rebin(const VariableConstView &var, const Dim dim,
               const VariableConstView &oldCoord,
               const VariableConstView &newCoord) {
  // Rebin could also implemented for count-densities. However, it may be better
  // to avoid this since it increases complexity. Instead, densities could
  // always be computed on-the-fly for visualization, if required.
  core::expect::unit_any_of(var, {units::counts, units::one});
  if (!isBinEdge(dim, oldCoord.dims(), var.dims()))
    throw except::BinEdgeError(
        "The input does not have coordinates with bin-edges.");

  if (var.dims().inner() == dim) {
    using namespace rebin_inner_detail;
    return transform_subspan<std::tuple<
        args<double, double, double, double>, args<float, float, float, float>,
        args<float, double, float, double>, args<float, float, float, double>,
        args<bool, double, bool, double>>>(var.dtype(), dim,
                                           newCoord.dims()[dim] - 1, newCoord,
                                           var, oldCoord, core::element::rebin);

  } else {
    auto dims = var.dims();
    dims.resize(dim, newCoord.dims()[dim] - 1);
    Variable rebinned(var, dims);
    if (newCoord.dims().ndim() > 1)
      throw std::runtime_error(
          "Not inner rebin works only for 1d coordinates for now.");
    if (oldCoord.dtype() == dtype<double>)
      rebin_non_inner<double>(dim, var, rebinned, oldCoord, newCoord);
    else if (oldCoord.dtype() == dtype<float>)
      rebin_non_inner<float>(dim, var, rebinned, oldCoord, newCoord);
    else
      throw std::runtime_error(
          "Rebinning is possible only for double and float types.");
    return rebinned;
  }
}

} // namespace scipp::variable
