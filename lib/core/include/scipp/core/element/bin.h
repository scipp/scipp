// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>
#include <limits>
#include <numeric>

#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/util.h"
#include "scipp/core/histogram.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

template <class Index, class Coord, class Edges = Coord>
using update_indices_by_binning_arg =
    std::tuple<Index, Coord, scipp::span<const Edges>>;

static constexpr auto update_indices_by_binning = overloaded{
    element::arg_list<update_indices_by_binning_arg<int64_t, double>,
                      update_indices_by_binning_arg<int32_t, double>,
                      update_indices_by_binning_arg<int64_t, float>,
                      update_indices_by_binning_arg<int32_t, float>,
                      update_indices_by_binning_arg<int64_t, int64_t>,
                      update_indices_by_binning_arg<int32_t, int64_t>,
                      update_indices_by_binning_arg<int64_t, int32_t>,
                      update_indices_by_binning_arg<int32_t, int32_t>,
                      update_indices_by_binning_arg<int64_t, time_point>,
                      update_indices_by_binning_arg<int32_t, time_point>,
                      update_indices_by_binning_arg<int64_t, int64_t, double>,
                      update_indices_by_binning_arg<int32_t, int64_t, double>,
                      update_indices_by_binning_arg<int64_t, int32_t, double>,
                      update_indices_by_binning_arg<int32_t, int32_t, double>,
                      update_indices_by_binning_arg<int64_t, float, double>,
                      update_indices_by_binning_arg<int32_t, float, double>,
                      update_indices_by_binning_arg<int64_t, int32_t, int64_t>,
                      update_indices_by_binning_arg<int32_t, int32_t, int64_t>>,
    [](units::Unit &indices, const units::Unit &coord,
       const units::Unit &groups) {
      expect::equals(coord, groups);
      expect::equals(indices, units::none);
    },
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<2>};

// Special faster implementation for linear bins.
static constexpr auto update_indices_by_binning_linspace =
    overloaded{update_indices_by_binning,
               [](auto &index, const auto &x, const auto &edges) {
                 if (index == -1)
                   return;
                 const auto [offset, nbin, scale] =
                     core::linear_edge_params(edges);
                 using Index = std::decay_t<decltype(index)>;
                 Index bin = (x - offset) * scale;
                 bin = std::clamp(bin, Index(0), Index(nbin - 1));
                 index *= nbin;
                 if (x < edges[bin]) {
                   if (bin != 0 && x >= edges[bin - 1])
                     index += bin - 1;
                   else
                     index = -1;
                 } else if (x >= edges[bin + 1]) {
                   if (bin != nbin - 1)
                     index += bin + 1;
                   else
                     index = -1;
                 } else {
                   index += bin;
                 }
               }};

static constexpr auto update_indices_by_binning_sorted_edges =
    overloaded{update_indices_by_binning,
               [](auto &index, const auto &x, const auto &edges) {
                 if (index == -1)
                   return;
                 auto it = std::upper_bound(edges.begin(), edges.end(), x);
                 index *= scipp::size(edges) - 1;
                 if (it == edges.begin() || it == edges.end()) {
                   index = -1;
                 } else {
                   index += --it - edges.begin();
                 }
               }};

template <class Index>
static constexpr auto groups_to_map = overloaded{
    element::arg_list<scipp::span<const double>, scipp::span<const float>,
                      scipp::span<const int64_t>, scipp::span<const int32_t>,
                      scipp::span<const bool>, scipp::span<const std::string>,
                      scipp::span<const time_point>>,
    transform_flags::expect_no_variance_arg<0>,
    [](const units::Unit &u) { return u; },
    [](const auto &groups) {
      std::unordered_map<typename std::decay_t<decltype(groups)>::value_type,
                         Index>
          index;
      scipp::index current = 0;
      for (const auto &item : groups)
        index[item] = current++;
      if (scipp::size(groups) != scipp::size(index))
        throw std::runtime_error("Duplicate group labels.");
      return index;
    }};

template <class Index, class T>
using update_indices_by_grouping_arg =
    std::tuple<Index, T, std::unordered_map<T, Index>>;

static constexpr auto update_indices_by_grouping = overloaded{
    element::arg_list<update_indices_by_grouping_arg<int64_t, double>,
                      update_indices_by_grouping_arg<int32_t, double>,
                      update_indices_by_grouping_arg<int64_t, float>,
                      update_indices_by_grouping_arg<int32_t, float>,
                      update_indices_by_grouping_arg<int64_t, int64_t>,
                      update_indices_by_grouping_arg<int32_t, int64_t>,
                      update_indices_by_grouping_arg<int64_t, int32_t>,
                      update_indices_by_grouping_arg<int32_t, int32_t>,
                      update_indices_by_grouping_arg<int64_t, bool>,
                      update_indices_by_grouping_arg<int32_t, bool>,
                      update_indices_by_grouping_arg<int64_t, std::string>,
                      update_indices_by_grouping_arg<int32_t, std::string>,
                      update_indices_by_grouping_arg<int32_t, time_point>,
                      update_indices_by_grouping_arg<int64_t, time_point>>,
    [](units::Unit &indices, const units::Unit &coord,
       const units::Unit &groups) {
      expect::equals(coord, groups);
      expect::equals(indices, units::none);
    },
    [](auto &index, const auto &x, const auto &groups) {
      if (index == -1)
        return;
      const auto it = groups.find(x);
      index *= scipp::size(groups);
      index = (it == groups.end()) ? -1 : (index + it->second);
    }};

static constexpr auto update_indices_from_existing = overloaded{
    element::arg_list<std::tuple<int64_t, scipp::index, scipp::index>,
                      std::tuple<int32_t, scipp::index, scipp::index>>,
    [](units::Unit &, const units::Unit &, const units::Unit &) {},
    [](auto &index, const auto bin_index, const auto nbin) {
      if (index == -1)
        return;
      index *= nbin;
      index += bin_index;
    }};

static constexpr auto count_indices = overloaded{
    element::arg_list<
        std::tuple<scipp::span<const int64_t>, scipp::index, scipp::index>,
        std::tuple<scipp::span<const int32_t>, scipp::index, scipp::index>>,
    [](const units::Unit &indices, const auto &offset, const auto &nbin) {
      expect::equals(indices, units::none);
      expect::equals(offset, units::none);
      expect::equals(nbin, units::none);
      return units::none;
    },
    [](const auto &indices, const auto offset, const auto nbin) {
      typename SubbinSizes::container_type counts(nbin);
      for (const auto i : indices)
        if (i >= 0)
          ++counts[i];
      return SubbinSizes{offset, std::move(counts)};
    }};

} // namespace scipp::core::element
