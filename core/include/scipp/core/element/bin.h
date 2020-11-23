// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cmath>
#include <numeric>

#include "scipp/common/overloaded.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/util.h"
#include "scipp/core/histogram.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

template <class T>
using update_indices_by_binning_arg =
    std::tuple<scipp::index, T, scipp::span<const T>>;

static constexpr auto update_indices_by_binning =
    overloaded{element::arg_list<update_indices_by_binning_arg<double>,
                                 update_indices_by_binning_arg<float>>,
               [](units::Unit &indices, const units::Unit &coord,
                  const units::Unit &groups) {
                 expect::equals(coord, groups);
                 expect::equals(indices, units::one);
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
                 const double bin = (x - offset) * scale;
                 index *= scipp::size(edges) - 1;
                 index += (bin < 0.0 || bin >= nbin) ? -1 : bin;
               }};

static constexpr auto update_indices_by_binning_sorted_edges =
    overloaded{update_indices_by_binning,
               [](auto &index, const auto &x, const auto &edges) {
                 if (index == -1)
                   return;
                 auto it = std::upper_bound(edges.begin(), edges.end(), x);
                 index *= scipp::size(edges) - 1;
                 index += (it == edges.begin() || it == edges.end())
                              ? -1
                              : --it - edges.begin();
               }};

static constexpr auto groups_to_map = overloaded{
    element::arg_list<span<const int64_t>, span<const int32_t>,
                      span<const std::string>>,
    transform_flags::expect_no_variance_arg<0>,
    [](const units::Unit &u) { return u; },
    [](const auto &groups) {
      std::unordered_map<typename std::decay_t<decltype(groups)>::value_type,
                         scipp::index>
          index;
      scipp::index current = 0;
      for (const auto &item : groups)
        index[item] = current++;
      if (scipp::size(groups) != scipp::size(index))
        throw std::runtime_error("Duplicate group labels.");
      return index;
    }};

template <class T>
using update_indices_by_grouping_arg =
    std::tuple<scipp::index, T, std::unordered_map<T, scipp::index>>;

static constexpr auto update_indices_by_grouping =
    overloaded{element::arg_list<update_indices_by_grouping_arg<int64_t>,
                                 update_indices_by_grouping_arg<int32_t>,
                                 update_indices_by_grouping_arg<std::string>>,
               [](units::Unit &indices, const units::Unit &coord,
                  const units::Unit &groups) {
                 expect::equals(coord, groups);
                 expect::equals(indices, units::one);
               },
               [](auto &index, const auto &x, const auto &groups) {
                 if (index == -1)
                   return;
                 const auto it = groups.find(x);
                 index *= scipp::size(groups);
                 index += it == groups.end() ? -1 : it->second;
               }};

static constexpr auto update_indices_from_existing =
    overloaded{element::arg_list<scipp::index>,
               [](units::Unit &, const units::Unit &, const units::Unit &) {},
               [](auto &index, const auto bin_index, const auto nbin) {
                 if (index == -1)
                   return;
                 index *= nbin;
                 index += bin_index;
               }};

// - Each span covers an *input* bin.
// - `offsets` Start indices of the output bins
// - `bin_indices` Target output bin index (within input bin)
template <class T>
using bin_arg = std::tuple<span<T>, span<const scipp::index>, span<const T>,
                           span<const scipp::index>>;
static constexpr auto bin = overloaded{
    element::arg_list<bin_arg<double>, bin_arg<float>, bin_arg<int64_t>,
                      bin_arg<int32_t>, bin_arg<bool>, bin_arg<Eigen::Vector3d>,
                      bin_arg<std::string>>,
    transform_flags::expect_in_variance_if_out_variance,
    [](units::Unit &binned, const units::Unit &, const units::Unit &data,
       const units::Unit &) { binned = data; },
    [](const auto &binned, const auto &offsets, const auto &data,
       const auto &bin_indices) {
      std::vector<scipp::index> bins(offsets.begin(), offsets.end());
      const auto size = scipp::size(bin_indices);
      using T = std::decay_t<decltype(data)>;
      for (scipp::index i = 0; i < size; ++i) {
        const auto i_bin = bin_indices[i];
        if (i_bin < 0)
          continue;
        if constexpr (is_ValueAndVariance_v<T>) {
          binned.variance[bins[i_bin]] = data.variance[i];
          binned.value[bins[i_bin]++] = data.value[i];
        } else {
          binned[bins[i_bin]++] = data[i];
        }
      }
    }};

static constexpr auto count_indices = overloaded{
    element::arg_list<
        std::tuple<scipp::span<scipp::index>, scipp::span<const scipp::index>>>,
    [](const units::Unit &counts, const units::Unit &indices) {
      expect::equals(indices, units::one);
      expect::equals(counts, units::one);
    },
    [](const auto &counts, const auto &indices) {
      zero(counts);
      for (const auto i : indices)
        if (i >= 0)
          ++counts[i];
    }};

} // namespace scipp::core::element
