// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

constexpr auto logical =
    overloaded{arg_list<bool>, dimensionless_unit_check_return};

constexpr auto logical_and =
    overloaded{logical, [](const auto &a, const auto &b) { return a && b; }};
constexpr auto logical_or =
    overloaded{logical, [](const auto &a, const auto &b) { return a || b; }};
constexpr auto logical_xor =
    overloaded{logical, [](const auto &a, const auto &b) { return a != b; }};
constexpr auto logical_not =
    overloaded{logical, [](const auto &x) { return !x; }};

constexpr auto logical_inplace =
    overloaded{arg_list<bool>, dimensionless_unit_check};

constexpr auto logical_and_equals =
    overloaded{logical_inplace, [](auto &&a, const auto &b) { a = a && b; }};
constexpr auto logical_or_equals =
    overloaded{logical_inplace, [](auto &&a, const auto &b) { a = a || b; }};
constexpr auto logical_xor_equals =
    overloaded{logical_inplace, [](auto &&a, const auto &b) { a = a != b; }};

} // namespace scipp::core::element
