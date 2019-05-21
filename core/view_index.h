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
  ViewIndex(const Dimensions &parentDimensions,
            const Dimensions &subdimensions) {
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
  scipp::index m_index{0};
  scipp::index m_delta[NDIM_MAX] = {0, 0, 0, 0, 0, 0};
  scipp::index m_coord[NDIM_MAX] = {0, 0, 0, 0, 0, 0};
  scipp::index m_extent[NDIM_MAX] = {0, 0, 0, 0, 0, 0};

  scipp::index m_fullIndex;
  int32_t m_dims;
  int32_t m_subdims;
  int32_t m_offsets[NDIM_MAX];
  scipp::index m_factors[NDIM_MAX];
};

} // namespace scipp::core

#endif // VIEW_INDEX_H
