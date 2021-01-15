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

template <class Coord, class Edge>
using bin_range_arg =
    std::tuple<scipp::index, scipp::index, Coord, scipp::span<const Edge>>;

static constexpr auto bin_range_common = overloaded{
    arg_list<bin_range_arg<double, double>, bin_range_arg<int64_t, double>>,
    transform_flags::expect_no_variance_arg<2>};

static constexpr auto begin_bin =
    overloaded{bin_range_common, [](auto &bin, auto &index, const auto &coord,
                                    const auto &edges) {
                 while (bin + 2 < scipp::size(edges) && edges[bin + 1] <= coord)
                   ++bin;
                 index = bin;
               }};

static constexpr auto end_bin =
    overloaded{bin_range_common, [](auto &bin, auto &index, const auto &coord,
                                    const auto &edges) {
                 while (bin + 2 < scipp::size(edges) && edges[bin + 1] < coord)
                   ++bin;
                 index = bin + 1;
               }};

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
