// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <numeric>
#include <span>

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
    std::tuple<Index, Coord, std::span<const Edges>>;

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
                      update_indices_by_binning_arg<int64_t, double, float>,
                      update_indices_by_binning_arg<int32_t, double, float>,
                      update_indices_by_binning_arg<int64_t, int32_t, int64_t>,
                      update_indices_by_binning_arg<int32_t, int32_t, int64_t>>,
    // `indices` must be non-const so `auto &index` overloads below don't match.
    // cppcheck-suppress constParameterReference
    [](sc_units::Unit &indices, const sc_units::Unit &coord,
       const sc_units::Unit &groups) {
      expect::equals(coord, groups);
      expect::equals(sc_units::none, indices);
    },
    transform_flags::expect_no_variance_arg<1>,
    transform_flags::expect_no_variance_arg<2>};

// Special faster implementation for linear bins.
static constexpr auto update_indices_by_binning_linspace = overloaded{
    update_indices_by_binning,
    [](auto &index, const auto &x, const auto &edges) {
      if (index == -1)
        return;
      using Index = std::decay_t<decltype(index)>;
      const auto params = core::linear_edge_params(edges);
      if (const auto bin = get_bin<Index>(x, edges, params); bin < 0) {
        index = -1;
      } else {
        index *= std::get<1>(params); // nbin
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
    element::arg_list<std::span<const double>, std::span<const float>,
                      std::span<const int64_t>, std::span<const int32_t>,
                      std::span<const bool>, std::span<const std::string>,
                      std::span<const time_point>>,
    transform_flags::expect_no_variance_arg<0>,
    [](const sc_units::Unit &u) { return u; },
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

template <class Index, class Coord, class Edges = Coord>
using update_indices_by_grouping_arg =
    std::tuple<Index, Coord, std::unordered_map<Edges, Index>>;

static constexpr auto update_indices_by_grouping = overloaded{
    element::arg_list<update_indices_by_grouping_arg<int64_t, double>,
                      update_indices_by_grouping_arg<int32_t, double>,
                      update_indices_by_grouping_arg<int64_t, float>,
                      update_indices_by_grouping_arg<int32_t, float>,
                      update_indices_by_grouping_arg<int64_t, int64_t>,
                      update_indices_by_grouping_arg<int32_t, int64_t>,
                      // Given int32 target groups, select from int64. Note that
                      // we do not support the reverse for now, since the
                      // `groups.find(x)` below would then have to cast to a
                      // lower precision, i.e., we would need special handling.
                      update_indices_by_grouping_arg<int64_t, int64_t, int32_t>,
                      update_indices_by_grouping_arg<int32_t, int64_t, int32_t>,
                      update_indices_by_grouping_arg<int64_t, int32_t>,
                      update_indices_by_grouping_arg<int32_t, int32_t>,
                      update_indices_by_grouping_arg<int32_t, int32_t, int64_t>,
                      update_indices_by_grouping_arg<int64_t, bool>,
                      update_indices_by_grouping_arg<int32_t, bool>,
                      update_indices_by_grouping_arg<int64_t, std::string>,
                      update_indices_by_grouping_arg<int32_t, std::string>,
                      update_indices_by_grouping_arg<int32_t, time_point>,
                      update_indices_by_grouping_arg<int64_t, time_point>>,
    // `indices` must be non-const so `auto &index` overloads below don't match.
    // cppcheck-suppress constParameterReference
    [](sc_units::Unit &indices, const sc_units::Unit &coord,
       const sc_units::Unit &groups) {
      expect::equals(coord, groups);
      expect::equals(sc_units::none, indices);
    },
    [](auto &index, const auto &x, const auto &groups) {
      if (index == -1)
        return;
      const auto it = groups.find(x);
      index *= scipp::size(groups);
      index = (it == groups.end()) ? -1 : (index + it->second);
    }};

template <class Index, class Coord, class Edges = Coord>
using update_indices_by_grouping_contiguous_arg =
    std::tuple<Index, Coord, scipp::index, Edges>;

static constexpr auto update_indices_by_grouping_contiguous = overloaded{
    element::arg_list<
        update_indices_by_grouping_contiguous_arg<int64_t, int64_t>,
        update_indices_by_grouping_contiguous_arg<int32_t, int64_t>,
        // Given int32 target groups, select from int64. Note that
        // we do not support the reverse for now, since the
        // `groups.find(x)` below would then have to cast to a
        // lower precision, i.e., we would need special handling.
        update_indices_by_grouping_contiguous_arg<int64_t, int64_t, int32_t>,
        update_indices_by_grouping_contiguous_arg<int32_t, int64_t, int32_t>,
        update_indices_by_grouping_contiguous_arg<int64_t, int32_t>,
        update_indices_by_grouping_contiguous_arg<int32_t, int32_t>,
        update_indices_by_grouping_contiguous_arg<int32_t, int32_t, int64_t>>,
    // `indices` must be non-const so `auto &index` overloads below don't match.
    // cppcheck-suppress constParameterReference
    [](sc_units::Unit &indices, const sc_units::Unit &coord,
       const sc_units::Unit &ngroup, const sc_units::Unit &offset) {
      expect::equals(coord, offset);
      expect::equals(sc_units::none, ngroup);
      expect::equals(sc_units::none, indices);
    },
    [](auto &index, const auto &x, const auto &ngroup, const auto &offset) {
      if (index == -1)
        return;
      index *= ngroup;
      const auto group = x - offset;
      index = group < 0 || group >= ngroup ? -1 : (index + group);
    }};

static constexpr auto update_indices_from_existing = overloaded{
    element::arg_list<std::tuple<int64_t, scipp::index, scipp::index>,
                      std::tuple<int32_t, scipp::index, scipp::index>>,
    [](sc_units::Unit &, const sc_units::Unit &, const sc_units::Unit &) {},
    [](auto &index, const auto bin_index, const auto nbin) {
      if (index == -1)
        return;
      index *= nbin;
      index += bin_index;
    }};

static constexpr auto count_indices = overloaded{
    element::arg_list<
        std::tuple<std::span<const int64_t>, scipp::index, scipp::index>,
        std::tuple<std::span<const int32_t>, scipp::index, scipp::index>>,
    [](const sc_units::Unit &indices, const auto &offset, const auto &nbin) {
      expect::equals(sc_units::none, indices);
      expect::equals(sc_units::none, offset);
      expect::equals(sc_units::none, nbin);
      return sc_units::none;
    },
    [](const auto &indices, const auto offset, const auto nbin) {
      typename SubbinSizes::container_type counts(nbin);
      for (const auto i : indices)
        if (i >= 0)
          ++counts[i];
      return SubbinSizes{offset, std::move(counts)};
    }};

} // namespace scipp::core::element
