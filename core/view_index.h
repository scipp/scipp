// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#ifndef VIEW_INDEX_H
#define VIEW_INDEX_H

#include "dimensions.h"

namespace scipp::core {

class ViewIndex {
public:
  ViewIndex(const Dimensions &targetDimensions,
            const Dimensions &dataDimensions) {
    m_dims = targetDimensions.shape().size();
    for (scipp::index d = 0; d < m_dims; ++d)
      m_extent[d] = targetDimensions.size(m_dims - 1 - d);
    scipp::index factor{1};
    for (scipp::index i = dataDimensions.shape().size() - 1; i >= 0; --i) {
      const auto dimension = dataDimensions.label(i);
      if (targetDimensions.contains(dimension)) {
        m_offsets[m_subdims] = m_dims - 1 - targetDimensions.index(dimension);
        m_factors[m_subdims] = factor;
        ++m_subdims;
      }
      factor *= dataDimensions.size(i);
    }
    scipp::index offset{1};
    for (scipp::index d = 0; d < m_dims; ++d) {
      setIndex(offset);
      m_delta[d] = m_index;
      if (d > 0) {
        setIndex(offset - 1);
        m_delta[d] -= m_index;
        for (scipp::index d2 = 0; d2 < d; ++d2)
          m_delta[d] -= m_delta[d2];
      }
      offset *= m_extent[d];
    }
    setIndex(0);
  }

  void increment() {
    m_index += m_delta[0];
    ++m_coord[0];
    scipp::index d = 0;
    while ((m_coord[d] == m_extent[d]) && (d < NDIM_MAX - 1)) {
      m_index += m_delta[d + 1];
      ++m_coord[d + 1];
      m_coord[d] = 0;
      ++d;
    }
    ++m_fullIndex;
  }

  void setIndex(const scipp::index index) {
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

  scipp::index get() const { return m_index; }
  scipp::index index() const { return m_fullIndex; }

  bool operator==(const ViewIndex &other) const {
    return m_fullIndex == other.m_fullIndex;
  }

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
  std::array<scipp::index, NDIM_MAX> m_delta = {0, 0, 0, 0, 0, 0};
  std::array<scipp::index, NDIM_MAX> m_coord = {0, 0, 0, 0, 0, 0};
  std::array<scipp::index, NDIM_MAX> m_extent = {0, 0, 0, 0, 0, 0};
  scipp::index m_fullIndex;
  int32_t m_dims;
  int32_t m_subdims = 0;
  std::array<int32_t, NDIM_MAX> m_offsets;
  std::array<scipp::index, NDIM_MAX> m_factors;
};

} // namespace scipp::core

#endif // VIEW_INDEX_H
