// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <limits>

#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/element/arg_list.h"
#include "scipp/core/element/util.h"
#include "scipp/core/histogram.h"
#include "scipp/core/subbin_sizes.h"
#include "scipp/core/time_point.h"
#include "scipp/core/transform_common.h"

namespace scipp::core::element {

auto map_to_bins_direct = [](auto &binned, auto &bins, const auto &data,
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

constexpr bool is_powerof2(int v) { return v && ((v & (v - 1)) == 0); }

template <int chunksize>
auto map_to_bins_chunkwise = [](auto &binned, auto &bins, const auto &data,
                                const auto &bin_indices) {
  // compiler is smart for div or mod 2**N, otherwise this would be too slow
  static_assert(is_powerof2(chunksize));
  using InnerIndex = int16_t;
  static_assert(chunksize <= std::numeric_limits<InnerIndex>::max());
  const auto size = scipp::size(bin_indices);
  using T = std::decay_t<decltype(data)>;

  using Val =
      std::conditional_t<is_ValueAndVariance_v<T>, typename T::value_type, T>;
  // TODO Ideally this buffer would be reused (on a per-thread basis)
  // for every application of the kernel.
  std::vector<std::tuple<std::vector<typename Val::value_type>,
                         std::vector<InnerIndex>>>
      chunks((bins.size() - 1) / chunksize + 1);
  for (scipp::index i = 0; i < size;) {
    // We operate in blocks so the size of the map of buffers, i.e.,
    // additional memory use of the algorithm, is bounded. This also
    // avoids costly allocations from resize operations.
    const scipp::index max = std::min(size, i + scipp::size(bins) * 8);
    // 1. Map to chunks
    for (; i < max; ++i) {
      const auto i_bin = bin_indices[i];
      if (i_bin < 0)
        continue;
      const InnerIndex j = i_bin % chunksize;
      const auto i_chunk = i_bin / chunksize;
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
        const auto i_bin = chunksize * i_chunk + ind[j];
        if constexpr (is_ValueAndVariance_v<T>) {
          binned.value[bins[i_bin]] = vals[2 * j];
          binned.variance[bins[i_bin]++] = vals[2 * j + 1];
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
using bin_arg = std::tuple<std::span<T>, SubbinSizes, std::span<const T>,
                           std::span<const Index>>;
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
    [](sc_units::Unit &binned, const sc_units::Unit &,
       const sc_units::Unit &data, const sc_units::Unit &) { binned = data; },
    [](const auto &binned, const auto &offsets, const auto &data,
       const auto &bin_indices) {
      auto bins(offsets.sizes());
      // If there are many bins, we have two performance issues:
      // 1. `bins` is large and will not fit into L1, L2, or L3 cache.
      // 2. Writes to output are very random, implying a cache miss for every
      //    event.
      // We can avoid some of this issue by first sorting into chunks, then
      // chunks into bins. For example, instead of mapping directly to 65536
      // bins, we may map to 256 chunks, and each chunk to 256 bins.
      const bool many_bins = bins.size() > 512;
      const bool multiple_events_per_bin = bins.size() * 4 < bin_indices.size();
      if (many_bins && multiple_events_per_bin) { // avoid overhead
        if (bins.size() <= 128 * 128)
          map_to_bins_chunkwise<128>(binned, bins, data, bin_indices);
        else if (bins.size() <= 256 * 256)
          map_to_bins_chunkwise<256>(binned, bins, data, bin_indices);
        else if (bins.size() <= 512 * 512)
          map_to_bins_chunkwise<512>(binned, bins, data, bin_indices);
        else
          map_to_bins_chunkwise<1024>(binned, bins, data, bin_indices);
      } else {
        map_to_bins_direct(binned, bins, data, bin_indices);
      }
    }};

} // namespace scipp::core::element
