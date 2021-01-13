// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

template <class Index, class T>
using update_indices_by_binning_arg =
    std::tuple<Index, T, scipp::span<const T>>;

static constexpr auto bin_index_sorted =
    overloaded{element::arg_list<std::tuple<double, span<const double>>>,
               [](const units::Unit &x, const units::Unit &edges) {
                 expect::equals(x, edges);
                 return units::one;
               },
               [](const auto &x, const auto &edges) {
                 for (scipp::index i = scipp::size(edges) - 1; i != 0; --i)
                   if (x >= edges[i])
                     return i;
                 return scipp::index{0};
               },
               transform_flags::expect_no_variance_arg<0>,
               transform_flags::expect_no_variance_arg<1>};

constexpr auto subbin_sizes_exclusive_scan =
    overloaded{arg_list<SubbinSizes>, [](auto &sum, auto &x) {
                 sum.trim_to(x);
                 sum += x;
                 x = sum - x;
               }};

constexpr auto subbin_sizes_add_intersection =
    overloaded{arg_list<SubbinSizes>,
               overloaded{[](units::Unit &a, const units::Unit &b) { a += b; },
                          [](auto &a, auto &b) { a.add_intersection(b); }}};

} // namespace scipp::core::element
