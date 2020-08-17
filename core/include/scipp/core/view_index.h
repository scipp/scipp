// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include "scipp-core_export.h"
#include "scipp/core/dimensions.h"

namespace scipp::core {

class SCIPP_CORE_EXPORT ViewIndex {
public:
  ViewIndex(const Dimensions &targetDimensions,
            const Dimensions &dataDimensions);
  ViewIndex(const Dimensions &targetDimensions,
            const Dimensions &dataDimensions, const Dimensions &nested,
            const Dim nested_dim,
            const scipp::span<const std::pair<scipp::index, scipp::index>>
                nested_ranges);

  void update_nested_range() {
    if (m_outer_index >= scipp::size(m_nested_ranges))
      return;
    const auto [begin, end] = m_nested_ranges[m_outer_index];
    m_index = begin;
    for (scipp::index dim = 0; dim < m_dim_nested; ++dim)
      m_index *= m_extent[dim]; // 2d or higher nested
    if (m_dim_nested + 1 != m_ndim_nested)
      m_delta[m_dim_nested + 1] += m_extent[m_dim_nested] - (end - begin);
    m_extent[m_dim_nested] = end - begin;
  }

  constexpr void increment_outer() noexcept {
    scipp::index d = 0;
    bool update_nested = false;
    while ((m_coord[d] == m_extent[d]) && (d < NDIM_MAX - 1)) {
      if (d + 1 >= m_ndim_nested) {
        m_outer_index += m_delta[d + 1];
        update_nested = true;
      } else {
        m_index += m_delta[d + 1];
      }
      ++m_coord[d + 1];
      m_coord[d] = 0;
      ++d;
    }
    if (update_nested)
      update_nested_range();
  }

  constexpr void increment() noexcept {
    m_index += m_delta[0];
    ++m_coord[0];
    if (m_coord[0] == m_extent[0])
      increment_outer();
    ++m_fullIndex;
  }

  constexpr void setIndex(const scipp::index index) noexcept {
    m_fullIndex = index;
    if (m_dims == 0)
      return;
    auto remainder{index};
    for (int32_t d = 0; d < m_dims - 1; ++d) {
      m_coord[d] = remainder % m_extent[d];
      remainder /= m_extent[d];
    }
    m_coord[m_dims - 1] = remainder;
    m_index = 0;
    for (int32_t j = 0; j < m_subdims; ++j)
      m_index += m_factors[j] * m_coord[m_offsets[j]];
  }

  constexpr scipp::index get() const noexcept { return m_index; }
  constexpr scipp::index index() const noexcept { return m_fullIndex; }

  constexpr bool operator==(const ViewIndex &other) const noexcept {
    return m_fullIndex == other.m_fullIndex;
  }
  constexpr bool operator!=(const ViewIndex &other) const noexcept {
    return m_fullIndex != other.m_fullIndex;
  }

  constexpr bool has_stride_zero() const noexcept { return m_dims > m_subdims; }

private:
  // NOTE:
  // We investigated different containers for the m_delta, m_coord & m_extent
  // arrays, and their impact on peformance when iterating over a variable
  // view.
  // Using std::array or C-style arrays give good performance (7.5 Gb/s) as long
  // as a range based loop is used:
  //
  //   for ( const auto x : view ) {
  //
  // If a loop which explicitly accesses the begin() and end() of the container
  // is used, e.g.
  //
  //   for ( auto it = view.begin(); it != view.end(); ++it ) {
  //
  // then the results differ widely.
  // - using std::array is 80x slower than above, at ~90 Mb/s
  // - using C-style arrays is 20x slower than above, at ~330 Mb/s
  //
  // We can recover the maximum performance by storing the view.end() in a
  // variable, e.g.
  //
  //   auto iend = view.end();
  //   for ( auto it = view.begin(); it != iend; ++it ) {
  //
  // for both std::array and C-style arrays.
  //
  // Finally, when using C-style arrays, we get a compilation warning from L37
  //
  //   m_delta[d] -= m_delta[d2];
  //
  // with the GCC compiler:
  //
  //  warning: array subscript is above array bounds [-Warray-bounds]
  //
  // which disappears when switching to std::array. This warning is not given
  // by the CLANG compiler, and is not fully understood as d2 is always less
  // than d and should never overflow the array bounds.
  // We decided to go with std::array as our final choice to avoid the warning,
  // as the performance is identical to C-style arrays, as long as range based
  // loops are used.
  scipp::index m_index{0};
  scipp::index m_outer_index{0}; // outer index in case of nesting
  std::array<scipp::index, NDIM_MAX> m_delta = {};
  std::array<scipp::index, NDIM_MAX> m_coord = {};
  std::array<scipp::index, NDIM_MAX> m_extent = {};
  scipp::index m_fullIndex;
  int32_t m_dims;
  int32_t m_subdims = 0;
  std::array<int32_t, NDIM_MAX> m_offsets;
  std::array<scipp::index, NDIM_MAX> m_factors;
  scipp::index m_ndim_nested{NDIM_MAX};
  scipp::span<const std::pair<scipp::index, scipp::index>> m_nested_ranges;
  scipp::index m_dim_nested{
      0}; // dimension index referred to by ranges, for nested > 1d
};

} // namespace scipp::core
