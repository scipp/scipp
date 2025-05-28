// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/dtype.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/except.h"
#include "scipp/units/unit.h"

namespace scipp::core::element {

template <class... Ts>
constexpr arg_list_t<std::tuple<event_list<Ts>, event_list<Ts>, bool>...>
    flatten_arg_list{};

constexpr auto flatten = overloaded{
    flatten_arg_list<double, float, int64_t, int32_t>,
    [](auto &a, const auto &b, const auto &mask) {
      if (mask)
        a.insert(a.end(), b.begin(), b.end());
    },
    [](sc_units::Unit &a, const sc_units::Unit &b, const sc_units::Unit &mask) {
      core::expect::equals(sc_units::one, mask);
      core::expect::equals(a, b);
    }};

} // namespace scipp::core::element
