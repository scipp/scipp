// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <numeric>

#include "scipp/common/overloaded.h"
#include "scipp/common/span.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {

constexpr auto exclusive_scan_bins = overloaded{
    arg_list<scipp::span<double>, scipp::span<float>, scipp::span<int64_t>,
             scipp::span<int32_t>>,
    transform_flags::expect_no_variance_arg<0>, [](units::Unit &) {},
    [](auto &x) { std::exclusive_scan(x.begin(), x.end(), x.begin(), 0); }};

constexpr auto exclusive_scan =
    overloaded{arg_list<int64_t, int32_t>, [](auto &sum, auto &x) {
                 sum += x;
                 x = sum - x;
               }};

} // namespace scipp::core::element
