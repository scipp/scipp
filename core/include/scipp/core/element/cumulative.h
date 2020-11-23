// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"

namespace scipp::core::element {

constexpr auto exclusive_scan =
    overloaded{arg_list<int64_t, int32_t>, [](auto &sum, auto &x) {
                 sum += x;
                 x = sum - x;
               }};

constexpr auto inclusive_scan =
    overloaded{arg_list<int64_t, int32_t>, [](auto &sum, auto &x) {
                 sum += x;
                 x = sum;
               }};

} // namespace scipp::core::element
