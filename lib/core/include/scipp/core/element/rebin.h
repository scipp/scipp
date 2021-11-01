// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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

template <class Less>
static constexpr auto rebin = overloaded{
    [](const auto &data_new, const auto &xnew, const auto &data_old,
       const auto &xold) {
      using T = decltype(xold[0] + (xold[0] - xnew[0]));
      // Note: using const rather than constexpr here is required to
      // avoid an internal compiler error on Windows/MSVC
      const Less less;
      zero(data_new);
      const auto oldSize = scipp::size(xold) - 1;
      const auto newSize = scipp::size(xnew) - 1;
      scipp::index iold = 0;
      scipp::index inew = 0;
      while ((iold < oldSize) && (inew < newSize)) {
        const T xo_low = xold[iold];
        const T xo_high = xold[iold + 1];
        const T xn_low = xnew[inew];
        const T xn_high = xnew[inew + 1];
        if (!less(xo_low, xn_high))
          inew++; // old and new bins do not overlap
        else if (!less(xn_low, xo_high))
          iold++; // old and new bins do not overlap
        else {
          // delta is the overlap of the bins on the x axis
          using std::min;
          using std::max;
          const auto delta =
              std::abs(min(xn_high, xo_high, less) - max(xn_low, xo_low, less));
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
          if (less(xo_high, xn_high)) {
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
      return data;
    },
    transform_flags::expect_in_variance_if_out_variance,
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<3>};

} // namespace scipp::core::element
