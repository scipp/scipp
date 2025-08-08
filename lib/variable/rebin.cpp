// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock, Igor Gudich
#include "scipp/core/element/rebin.h"
#include "scipp/core/parallel.h"
#include "scipp/units/except.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/rebin.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/transform_subspan.h"
#include "scipp/variable/util.h"

using namespace scipp::core::element;

namespace scipp::variable {

template <typename T, class Less>
void rebin_non_inner(const Dim dim, const Variable &oldT,
                     // cppcheck-suppress constParameter # bug in cppcheck
                     const Variable &newT, const Variable &oldCoord,
                     const Variable &newCoord) {
  if (oldCoord.ndim() != 1 || newCoord.ndim() != 1)
    throw std::invalid_argument(
        "Internal error in rebin, this should be unreachable.");
  const auto oldSize = oldT.dims()[dim];
  const auto newSize = newT.dims()[dim];

  const auto *xold = oldCoord.values<T>().data();
  const auto *xnew = newCoord.values<T>().data();

  auto add_from_bin = [&](auto &&slice, const auto xn_low, const auto xn_high,
                          const scipp::index iold) {
    auto xo_low = xold[iold];
    auto xo_high = xold[iold + 1];
    // delta is the overlap of the bins on the x axis
    const auto delta = std::abs(std::min<double>(xn_high, xo_high, Less{}) -
                                std::max<double>(xn_low, xo_low, Less{}));
    const auto owidth = std::abs(xo_high - xo_low);
    slice += oldT.slice({dim, iold}) *
             ((delta / owidth) * (slice.unit() / oldT.unit()));
  };
  auto accumulate_bin = [&](auto &&slice, const auto xn_low,
                            const auto xn_high) {
    scipp::index begin =
        std::upper_bound(xold, xold + oldSize + 1, xn_low, Less{}) - xold;
    scipp::index end =
        std::upper_bound(xold, xold + oldSize + 1, xn_high, Less{}) - xold;
    if (begin == oldSize + 1 || end == 0)
      return;
    begin = std::max(scipp::index(0), begin - 1);
    add_from_bin(slice, xn_low, xn_high, begin);
    if (begin + 1 < end - 1)
      sum_into(slice, oldT.slice({dim, begin + 1, end - 1}));
    if (begin != end - 1 && end < oldSize + 1)
      add_from_bin(slice, xn_low, xn_high, end - 1);
  };
  auto accumulate_bins = [&](const auto &range) {
    for (scipp::index inew = range.begin(); inew < range.end(); ++inew) {
      auto xn_low = xnew[inew];
      auto xn_high = xnew[inew + 1];
      accumulate_bin(newT.slice({dim, inew}), xn_low, xn_high);
    }
  };
  core::parallel::parallel_for(core::parallel::blocked_range(0, newSize),
                               accumulate_bins);
}

namespace {
template <class Out, class OutEdge, class In, class InEdge>
using args = std::tuple<std::span<Out>, std::span<const OutEdge>,
                        std::span<const In>, std::span<const InEdge>>;

struct Greater {
  template <class A, class B>
  constexpr bool operator()(const A a, const B b) const noexcept {
    return a > b;
  }
};

struct Less {
  template <class A, class B>
  constexpr bool operator()(const A a, const B b) const noexcept {
    return a < b;
  }
};

} // namespace

Variable rebin(const Variable &var, const Dim dim, const Variable &oldCoord,
               const Variable &newCoord) {
  // The code branch dealing with non-stride-1 data cannot handle non-1D edges.
  // This is likely a rare case in practice so a slow transpose of input and
  // output should be sufficient for now.
  if (var.stride(dim) != 1 && (oldCoord.ndim() != 1 || newCoord.ndim() != 1))
    // We *copy* the transpose to ensure that memory order of dims matches input
    return copy(
        transpose(rebin(as_contiguous(var, dim), dim, oldCoord, newCoord),
                  var.dims().labels()));
  // Rebin could also implemented for count-densities. However, it may be better
  // to avoid this since it increases complexity. Instead, densities could
  // always be computed on-the-fly for visualization, if required.
  if (!is_edges(var.dims(), oldCoord.dims(), dim))
    throw except::BinEdgeError(
        "The input does not have coordinates with bin-edges.");

  if (is_bins(var))
    throw except::TypeError("The input variable cannot be binned data. Use "
                            "`bin` or `histogram` instead of `rebin`.");

  using transform_args = std::tuple<
      args<double, double, int64_t, double>,
      args<double, double, int32_t, double>,
      args<double, core::time_point, double, core::time_point>,
      args<float, core::time_point, float, core::time_point>,
      args<double, core::time_point, int64_t, core::time_point>,
      args<double, core::time_point, int32_t, core::time_point>,
      args<double, core::time_point, bool, core::time_point>,
      args<double, double, double, double>, args<float, float, float, float>,
      args<float, double, float, double>, args<float, float, float, double>,
      args<double, double, bool, double>>;

  const bool ascending = allsorted(oldCoord, dim, SortOrder::Ascending) &&
                         allsorted(newCoord, dim, SortOrder::Ascending);
  if (!ascending && !(allsorted(oldCoord, dim, SortOrder::Descending) &&
                      allsorted(newCoord, dim, SortOrder::Descending)))
    throw except::BinEdgeError(
        "Rebin: The old or new bin edges are not sorted.");
  const auto out_type = (is_int(var.dtype()) || var.dtype() == dtype<bool>)
                            ? dtype<double>
                            : var.dtype();
  // Both code branches below require stride 1 for input and output edges.
  const auto oldEdges = as_contiguous(oldCoord, dim);
  const auto newEdges = as_contiguous(newCoord, dim);
  Variable rebinned;
  if (var.stride(dim) == 1) {
    if (ascending) {
      rebinned = transform_subspan<transform_args>(
          out_type, dim, newEdges.dims()[dim] - 1, newEdges, var, oldEdges,
          core::element::rebin<Less>, "rebin");
    } else {
      rebinned = transform_subspan<transform_args>(
          out_type, dim, newEdges.dims()[dim] - 1, newEdges, var, oldEdges,
          core::element::rebin<Greater>, "rebin");
    }
  } else {
    auto dims = var.dims();
    dims.resize(dim, newEdges.dims()[dim] - 1);
    rebinned = Variable(astype(Variable(var, Dimensions{}), out_type), dims);
    if (newEdges.dims().ndim() > 1)
      throw std::runtime_error(
          "Not inner rebin works only for 1d coordinates for now.");
    if (oldEdges.dtype() == dtype<double>) {
      if (ascending)
        rebin_non_inner<double, Less>(dim, var, rebinned, oldEdges, newEdges);
      else
        rebin_non_inner<double, Greater>(dim, var, rebinned, oldEdges,
                                         newEdges);
    } else if (oldEdges.dtype() == dtype<float>) {
      if (ascending)
        rebin_non_inner<float, Less>(dim, var, rebinned, oldEdges, newEdges);
      else
        rebin_non_inner<float, Greater>(dim, var, rebinned, oldEdges, newEdges);
    } else {
      throw except::TypeError("Rebinning is possible only for coords of types "
                              "`float64` or `float32`.");
    }
  }
  // If rebinned dimension has stride 1 but is not an inner dimension then we
  // need to transpose the output of transform_subspan to retain the input
  // dimension order.
  return transpose(rebinned, var.dims().labels());
}

} // namespace scipp::variable
