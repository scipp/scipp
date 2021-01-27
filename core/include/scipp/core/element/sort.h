// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Thibault Chatel
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/common/span.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/comparison.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {

namespace {
template <class Compare> constexpr auto make_sort(Compare compare) {
  return overloaded{
      core::element::arg_list<span<int64_t>, span<int32_t>, span<double>,
                              span<float>, span<std::string>>,
      [](units::Unit &) {},
      [&compare](auto &range) {
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
