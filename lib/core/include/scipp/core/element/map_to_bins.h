// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/util.h"
#include "scipp/core/histogram.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

// - Each span covers an *input* bin.
// - `offsets` Start indices of the output bins
// - `bin_indices` Target output bin index (within input bin)
template <class T, class Index>
using bin_arg = std::tuple<scipp::span<T>, SubbinSizes, scipp::span<const T>,
                           scipp::span<const Index>>;
static constexpr auto bin = overloaded{
    element::arg_list<
        bin_arg<double, int64_t>, bin_arg<double, int32_t>,
        bin_arg<float, int64_t>, bin_arg<float, int32_t>,
        bin_arg<int64_t, int64_t>, bin_arg<int64_t, int32_t>,
        bin_arg<int32_t, int64_t>, bin_arg<int32_t, int32_t>,
        bin_arg<bool, int64_t>, bin_arg<bool, int32_t>,
        bin_arg<Eigen::Vector3d, int64_t>, bin_arg<Eigen::Vector3d, int32_t>,
        bin_arg<std::string, int64_t>, bin_arg<std::string, int32_t>,
        bin_arg<time_point, int64_t>, bin_arg<time_point, int32_t>>,
    transform_flags::expect_in_variance_if_out_variance,
    [](units::Unit &binned, const units::Unit &, const units::Unit &data,
       const units::Unit &) { binned = data; },
    [](const auto &binned, const auto &offsets, const auto &data,
       const auto &bin_indices) {
      auto bins(offsets.sizes());
      const auto size = scipp::size(bin_indices);
      using T = std::decay_t<decltype(data)>;
      for (scipp::index i = 0; i < size; ++i) {
        const auto i_bin = bin_indices[i];
        if (i_bin < 0)
          continue;
        if constexpr (is_ValueAndVariance_v<T>) {
          binned.variance[bins[i_bin]] = data.variance[i];
          binned.value[bins[i_bin]++] = data.value[i];
        } else {
          binned[bins[i_bin]++] = data[i];
        }
      }
    }};

} // namespace scipp::core::element
