// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"

namespace scipp::core::element {

constexpr auto permute = overloaded{
    transform_flags::expect_no_variance_arg<1>,
    arg_list<std::tuple<scipp::span<const double>, scipp::index>,
             std::tuple<scipp::span<const float>, scipp::index>,
             std::tuple<scipp::span<const int64_t>, scipp::index>,
             std::tuple<scipp::span<const int32_t>, scipp::index>,
             std::tuple<scipp::span<const bool>, scipp::index>,
             std::tuple<scipp::span<const time_point>, scipp::index>,
             std::tuple<scipp::span<const std::string>, scipp::index>>,
    [](const auto &data, const auto &i) {
      if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(data)>>)
        return ValueAndVariance{data.value[i], data.variance[i]};
      else
        return data[i];
    },
    [](const units::Unit &data, const units::Unit &i) {
      expect::equals(i, units::one);
      return data;
    }};

} // namespace scipp::core::element
