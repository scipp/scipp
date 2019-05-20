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
  ViewIndex(
      const Dimensions &parentDimensions,
      const Dimensions &subdimensions) {
    if (parentDimensions.shape().size() > 4)
      throw std::runtime_error("ViewIndex supports at most 4 dimensions.");
    m_dims = parentDimensions.shape().size();
    for (scipp::index d = 0; d < m_dims; ++d)
      m_extent[d] = parentDimensions.size(m_dims - 1 - d);
    scipp::index factor{1};
    int32_t k = 0;
    for (scipp::index i = subdimensions.shape().size() - 1; i >= 0; --i) {
      const auto dimension = subdimensions.label(i);
      if (parentDimensions.contains(dimension)) {
        m_offsets[k] = m_dims - 1 - parentDimensions.index(dimension);
        m_factors[k] = factor;
        ++k;
      }
      factor *= subdimensions.size(i);
    }
    m_subdims = k;
    scipp::index offset{1};
    for (scipp::index d = 0; d < m_dims; ++d) {
      setIndex(offset);
      m_delta[d] = get();
      if (d > 0) {
        setIndex(offset - 1);
        m_delta[d] -= get();
      }
      for (scipp::index d2 = 0; d2 < d; ++d2)
        m_delta[d] -= m_delta[d2];
      offset *= m_extent[d];
    }
    setIndex(0);
  }

  void increment() {
    m_index += m_delta[0];
    ++m_coord[0];
    // It may seem counter-intuitive, but moving the code for a wrapped index
    // into a separate method helps with inlining of this *outer* part of the
    // increment method. Since mostly we do not wrap, inlining `increment()` is
    // the important part, the function call to `indexWrapped()` is not so
    // critical.
    if (m_coord[0] == m_extent[0])
      indexWrapped();
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
  void indexWrapped() {
    m_index += m_delta[1];
    m_coord[0] = 0;
    ++m_coord[1];
    if (m_coord[1] == m_extent[1]) {
      m_index += m_delta[2];
      m_coord[1] = 0;
      ++m_coord[2];
      if (m_coord[2] == m_extent[2]) {
        m_index += m_delta[3];
        m_coord[2] = 0;
        ++m_coord[3];
      }
    }
  }

  scipp::index m_index{0};
  alignas(32) scipp::index m_delta[4]{0, 0, 0, 0};
  alignas(32) scipp::index m_coord[4]{0, 0, 0, 0};
  alignas(32) scipp::index m_extent[4]{0, 0, 0, 0};
  scipp::index m_fullIndex;
  int32_t m_dims;
  int32_t m_numberOfSubindices;
  int32_t m_subdims;
  std::array<int32_t, 4> m_offsets;
  std::array<scipp::index, 4> m_factors;
};

} // namespace scipp::core

#endif // VIEW_INDEX_H
