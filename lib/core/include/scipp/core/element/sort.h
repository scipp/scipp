// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Thibault Chatel
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/comparison.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {

namespace {
template <class Compare> constexpr auto make_sort(Compare compare) {
  return overloaded{
      core::element::arg_list<std::span<int64_t>, std::span<int32_t>,
                              std::span<double>, std::span<float>,
                              std::span<std::string>, std::span<time_point>>,
      [](sc_units::Unit &) {},
      [compare](auto &range) {
        using T = std::decay_t<decltype(range)>;
        constexpr bool vars = is_ValueAndVariance_v<T>;
        if constexpr (vars) {
          std::vector<ValueAndVariance<typename T::value_type::value_type>>
              zipped;
          for (scipp::index i = 0; i < scipp::size(range.value); i++) {
            zipped.emplace_back(range.value[i], range.variance[i]);
          }
          std::sort(std::begin(zipped), std::end(zipped), compare);
          for (scipp::index i = 0; i < scipp::size(range.value); i++) {
            range.value[i] = zipped[i].value;
            range.variance[i] = zipped[i].variance;
          }
        } else {
          std::sort(range.begin(), range.end(), compare);
        }
      }};
}
} // namespace

auto sort_nonascending = make_sort(greater);
auto sort_nondescending = make_sort(less);

} // namespace scipp::core::element
