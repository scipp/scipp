// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

constexpr auto logical_and_equals =
    overloaded{arg_list<bool>, dimensionless_unit_check,
               [](auto &&a, const auto &b) { a &= b; }};

constexpr auto logical_or_equals =
    overloaded{arg_list<bool>, dimensionless_unit_check,
               [](auto &&a, const auto &b) { a |= b; }};

constexpr auto logical_xor_equals =
    overloaded{arg_list<bool>, dimensionless_unit_check,
               [](auto &&a, const auto &b) { a ^= b; }};

constexpr auto logical_and = overloaded{
    arg_list<bool>, dimensionless_unit_check_return,
    [](const auto &var_, const auto &other_) { return var_ & other_; }};

constexpr auto logical_or = overloaded{
    arg_list<bool>, dimensionless_unit_check_return,
    [](const auto &var_, const auto &other_) { return var_ | other_; }};

constexpr auto logical_xor = overloaded{
    arg_list<bool>, dimensionless_unit_check_return,
    [](const auto &var_, const auto &other_) { return var_ ^ other_; }};

constexpr auto logical_not =
    overloaded{arg_list<bool>, dimensionless_unit_check_return,
               [](const auto &current) { return !current; }};

} // namespace scipp::core::element
