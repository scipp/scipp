// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

// Cannot support variances since transform copies value and variance into
// ValueAndVariance before calling the functor. Only the *first* arg in an
// in-place transform is copied back to the input.
constexpr auto exclusive_scan = overloaded{
    arg_list<double, std::tuple<double, float>, int64_t, int32_t>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](auto &sum, auto &x) {
      sum += x;
      x = sum - x;
    }};

constexpr auto inclusive_scan = overloaded{
    arg_list<double, std::tuple<double, float>, int64_t, int32_t>,
    transform_flags::expect_no_variance_arg<0>,
    transform_flags::expect_no_variance_arg<1>, [](auto &sum, auto &x) {
      sum += x;
      x = sum;
    }};

} // namespace scipp::core::element
