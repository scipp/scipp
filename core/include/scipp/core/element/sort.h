// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Thibault Chatel
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/common/span.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

#include <stddef.h>

namespace scipp::core::element {

constexpr auto sort_common = overloaded{
    core::element::arg_list<span<int64_t>, span<int32_t>, span<double>,
                            span<float>, span<std::string>>,
    [](units::Unit &) {}};

template <typename F1, typename F2>
constexpr auto sort_to_specify(F1 for_zip, F2 for_no_var) {
  return overloaded{
      sort_common, [&for_zip, &for_no_var](auto &range) {
        using T = std::decay_t<decltype(range)>;
        constexpr bool vars = is_ValueAndVariance_v<T>;
        if constexpr (vars) {
          using B = typename T::value_type;
          using A = typename B::value_type;
          std::vector<std::pair<A, A>> zipped;
          for (scipp::index i = 0; i < scipp::size(range.value); i++) {
            zipped.push_back(std::make_pair(range.value[i], range.variance[i]));
          }
          std::sort(std::begin(zipped), std::end(zipped), for_zip);
          for (scipp::index i = 0; i < scipp::size(range.value); i++) {
            range.value[i] = zipped[i].first;
            range.variance[i] = zipped[i].second;
          }
        } else {
          std::sort(range.begin(), range.end(), for_no_var);
        }
      }};
}

auto sort_nonascending = sort_to_specify(
    [](const auto &a, const auto &b) { return a.first > b.first; },
    [](const auto &a, const auto &b) { return a > b; });

auto sort_nondescending = sort_to_specify(
    [](const auto &a, const auto &b) { return a.first < b.first; },
    [](const auto &a, const auto &b) { return a < b; });

} // namespace scipp::core::element
