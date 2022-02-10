// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/util.h"
#include "scipp/core/histogram.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

auto map_to_bins_direct = [](const auto &binned, auto &bins, const auto &data,
                             const auto &bin_indices) {
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
};

auto map_to_bins_chunkwise = [](const auto &binned, auto &bins,
                                const auto &data, const auto &bin_indices) {
  const auto size = scipp::size(bin_indices);
  using T = std::decay_t<decltype(data)>;

  using Val =
      std::conditional_t<is_ValueAndVariance_v<T>, typename T::value_type, T>;
  // TODO Ideally this buffer would be reused (on a per-thread basis)
  // for every application of the kernel.
  std::vector<
      std::tuple<std::vector<typename Val::value_type>, std::vector<uint8_t>>>
      chunks;
  chunks.resize(bins.size() / 256);
  for (scipp::index i = 0; i < size;) {
    // We operate in blocks so the size of the map of buffers, i.e.,
    // additional memory use of the algorithm, is bounded. This also
    // avoids costly allocations from resize operations.
    scipp::index max = std::min(size, i + scipp::size(bins) * 64);
    // 1. Map to chunks
    for (; i < max; ++i) {
      const auto i_bin = bin_indices[i];
      if (i_bin < 0)
        continue;
      // TODO use sqrt rounded to power of 2
      const uint8_t j = i_bin % 256; // compiler is smart for mod 2**N
      const auto i_chunk = i_bin / 256;
      auto &[vals, ind] = chunks[i_chunk];
      if constexpr (is_ValueAndVariance_v<T>) {
        vals.emplace_back(data.value[i]);
        vals.emplace_back(data.variance[i]);
      } else {
        vals.emplace_back(data[i]);
      }
      ind.emplace_back(j);
    }
    // 2. Map chunks to bins
    for (scipp::index i_chunk = 0; i_chunk < scipp::size(chunks); ++i_chunk) {
      auto &[vals, ind] = chunks[i_chunk];
      for (scipp::index j = 0; j < scipp::size(ind); ++j) {
        const auto i_bin = 256 * i_chunk + ind[j];
        if constexpr (is_ValueAndVariance_v<T>) {
          binned.variance[bins[i_bin]] = vals[2 * j + 1];
          binned.value[bins[i_bin]++] = vals[2 * j];
        } else {
          binned[bins[i_bin]++] = vals[j];
        }
      }
      vals.clear();
      ind.clear();
    }
  }
};

// - Each span covers an *input* bin.
// - `offsets` Start indices of the output bins
// - `bin_indices` Target output bin index (within input bin)
template <class T, class Index>
using bin_arg = std::tuple<scipp::span<T>, SubbinSizes, scipp::span<const T>,
                           scipp::span<const Index>>;
static constexpr auto bin = overloaded{
    element::arg_list<
        bin_arg<double, int64_t>, bin_arg<double, int32_t>,
        bin_arg<float, int64_t>, bin_arg<float, int32_t>,
        bin_arg<int64_t, int64_t>, bin_arg<int64_t, int32_t>,
        bin_arg<int32_t, int64_t>, bin_arg<int32_t, int32_t>,
        bin_arg<bool, int64_t>, bin_arg<bool, int32_t>,
        bin_arg<Eigen::Vector3d, int64_t>, bin_arg<Eigen::Vector3d, int32_t>,
        bin_arg<std::string, int64_t>, bin_arg<std::string, int32_t>,
        bin_arg<time_point, int64_t>, bin_arg<time_point, int32_t>>,
    transform_flags::expect_in_variance_if_out_variance,
    [](units::Unit &binned, const units::Unit &, const units::Unit &data,
       const units::Unit &) { binned = data; },
    [](const auto &binned, const auto &offsets, const auto &data,
       const auto &bin_indices) {
      auto bins(offsets.sizes());
      const auto size = scipp::size(bin_indices);
      // If there are many bins, we have two performance issues:
      // 1. `bins` is large and will not fit into L1, L2, or L3 cache.
      // 2. Writes to output are very random, implying a cache miss for every
      //    event.
      // We can avoid some of this issue by first sorting into chunks, then
      // chunks into bins. For example, instead of mapping directly to 65536
      // bins, we may map to 256 chunks, and each chunk to 256 bins.
      // TODO Ideally we should recurse this approach, with optimal
      // intermediate chunk sizes. These will in principle depend on cache
      // sizes.
      if (bins.size() > 512 && size > 128 * 1024) { // avoid overhead
        map_to_bins_chunkwise(binned, bins, data, bin_indices);
      } else {
        map_to_bins_direct(binned, bins, data, bin_indices);
      }
    }};

} // namespace scipp::core::element
