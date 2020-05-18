// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

constexpr auto and_equals = overloaded{
    arg_list<bool>,
    [](auto &&a, const auto &b) noexcept(noexcept(a &= b)) { a &= b; }};

constexpr auto or_equals = overloaded{
    arg_list<bool>,
    [](auto &&a, const auto &b) noexcept(noexcept(a |= b)) { a |= b; }};

} // namespace scipp::core::element
