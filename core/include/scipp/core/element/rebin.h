// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>
#include <type_traits>

#include "scipp/common/numeric.h"
#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/util.h"
#include "scipp/core/histogram.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"

namespace scipp::core::element {

struct AscendingRebin {};
struct DescendingRebin {};

namespace {
void rebin_error() {
  throw std::runtime_error(
      "Unknown type of rebin operation. "
      "Possible branches are AscendingRebin and DescendingRebin.");
}
} // namespace

template <class RebinType, class OldType, class NewType>
bool compare_new_high_with_old_low(const NewType xn_high,
                                   const OldType xo_low) {
  if constexpr (std::is_same_v<RebinType, AscendingRebin>)
    return xn_high <= xo_low;
  else if constexpr (std::is_same_v<RebinType, DescendingRebin>)
    return xn_high >= xo_low;
  else
    rebin_error();
}

template <class RebinType, class OldType, class NewType>
bool compare_old_high_with_new_low(const OldType xo_high,
                                   const NewType xn_low) {
  if constexpr (std::is_same_v<RebinType, AscendingRebin>)
    return xo_high <= xn_low;
  else if constexpr (std::is_same_v<RebinType, DescendingRebin>)
    return xo_high >= xn_low;
  else
    rebin_error();
}

template <class RebinType, class OldType, class NewType>
double compute_delta(const NewType xn_high, const OldType xo_high,
                     const NewType xn_low, const OldType xo_low) {
  if constexpr (std::is_same_v<RebinType, AscendingRebin>)
    return std::min<double>(xn_high, xo_high) -
           std::max<double>(xn_low, xo_low);
  else if constexpr (std::is_same_v<RebinType, DescendingRebin>)
    return std::min<double>(xn_low, xo_low) -
           std::max<double>(xn_high, xo_high);
  else
    rebin_error();
}

template <class RebinType, class OldType>
double compute_owidth(const OldType xo_high, const OldType xo_low) {
  if constexpr (std::is_same_v<RebinType, AscendingRebin>)
    return xo_high - xo_low;
  else if constexpr (std::is_same_v<RebinType, DescendingRebin>)
    return xo_low - xo_high;
  else
    rebin_error();
}

template <class RebinType, class OldType, class NewType>
bool compare_new_high_with_old_high(const NewType xn_high,
                                    const OldType xo_high) {
  if constexpr (std::is_same_v<RebinType, AscendingRebin>)
    return xn_high > xo_high;
  else if constexpr (std::is_same_v<RebinType, DescendingRebin>)
    return xn_high < xo_high;
  else
    rebin_error();
}

template <class T>
static constexpr auto rebin = overloaded{
    [](const auto &data_new, const auto &xnew, const auto &data_old,
       const auto &xold) {
      zero(data_new);
      const auto oldSize = scipp::size(xold) - 1;
      const auto newSize = scipp::size(xnew) - 1;
      scipp::index iold = 0;
      scipp::index inew = 0;
      while ((iold < oldSize) && (inew < newSize)) {
        const auto xo_low = xold[iold];
        const auto xo_high = xold[iold + 1];
        const auto xn_low = xnew[inew];
        const auto xn_high = xnew[inew + 1];
        if (T{}(xo_low, xn_high))
          inew++; // old and new bins do not overlap
        else if (T{}(xn_low, xo_high))
          iold++; // old and new bins do not overlap
        else {
          // delta is the overlap of the bins on the x axis
          const auto delta = std::abs(std::max<double>(xn_high, xo_high, T{}) -
                                      std::min<double>(xn_low, xo_low, T{}));
          const auto owidth = std::abs(xo_high - xo_low);
          const auto scale = delta / owidth;
          if constexpr (is_ValueAndVariance_v<
                            std::decay_t<decltype(data_old)>>) {
            data_new.value[inew] += data_old.value[iold] * scale;
            data_new.variance[inew] += data_old.variance[iold] * scale;
          } else if constexpr (std::is_same_v<typename std::decay_t<decltype(
                                                  data_new)>::value_type,
                                              bool>) {
            static_cast<void>(scale);
            data_new[inew] = data_new[inew] || data_old[iold];
          } else {
            data_new[inew] += data_old[iold] * scale;
          }
          if (T{}(xn_high, xo_high)) {
            iold++;
          } else {
            inew++;
          }
        }
      }
    },
    [](const units::Unit &target_edges, const units::Unit &data,
       const units::Unit &edges) {
      if (target_edges != edges)
        throw except::UnitError(
            "Input and output bin edges must have the same unit.");
      if (data != units::counts && data != units::one)
        throw except::UnitError("Only count-data (units::counts or "
                                "units::dimensionless) can be rebinned.");
      return data;
    },
    transform_flags::expect_in_variance_if_out_variance,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<3>};

} // namespace scipp::core::element
