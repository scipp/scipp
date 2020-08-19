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
    core::element::arg_list<
        std::tuple<span<int64_t>, span<int32_t>,
        span<double>, span<float>,
        span<std::string>>>,
    [](units::Unit &) {}};

constexpr auto sort_nondescending = overloaded{
    sort_common, [](auto &range) {
      using T = std::decay_t<decltype(range)>;
      constexpr bool vars = is_ValueAndVariance_v<T>;
      if constexpr (vars) {
        using B = typename T::value_type;
        using A = typename B::value_type;
        std::vector<std::pair<A,A>> zipped;
        for(long unsigned int i = 0; i<range.value.size(); i++){
          zipped.push_back(std::make_pair(range.value[i], range.variance[i]));
        }
        std::sort(std::begin(zipped), std::end(zipped),
                  [&](const auto& a, const auto& b)
                    { return a.first < b.first;}
        );
        for(long unsigned int i = 0; i<range.value.size(); i++){
          range.value[i] = zipped[i].first;
          range.variance[i] = zipped[i].second;
        }
      } else {
        std::sort(range.begin(), range.end());
      }
    }};

constexpr auto sort_nonascending = overloaded{
    sort_common, [](auto &range) {
      using T = std::decay_t<decltype(range)>;
      constexpr bool vars = is_ValueAndVariance_v<T>;
      if constexpr (vars) {
        using B = typename T::value_type;
        using A = typename B::value_type;
        std::vector<std::pair<A,A>> zipped;
        for(long unsigned int i = 0; i<range.value.size(); i++){
          zipped.push_back(std::make_pair(range.value[i], range.variance[i]));
        }
        std::sort(std::begin(zipped), std::end(zipped),
                  [&](const auto& a, const auto& b)
                    { return a.first > b.first;}
        );
        for(long unsigned int i = 0; i<range.value.size(); i++){
          range.value[i] = zipped[i].first;
          range.variance[i] = zipped[i].second;
        }
      } else {
        std::sort(range.rbegin(), range.rend());
      }
    }};
} // namespace scipp::core::element

