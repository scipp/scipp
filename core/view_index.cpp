// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "scipp/core/view_index.h"

namespace scipp::core {
ViewIndex::ViewIndex(const Dimensions &targetDimensions,
                     const Dimensions &dataDimensions) {
  m_dims = static_cast<int32_t>(targetDimensions.shape().size());
  for (scipp::index d = 0; d < m_dims; ++d)
    m_extent[d] = targetDimensions.size(m_dims - 1 - d);
  scipp::index factor{1};
  for (scipp::index i = dataDimensions.shape().size() - 1; i >= 0; --i) {
    // Note the hidden behavior here: ElementArrayView may be relabeling certain
    // dimensions to Dim::Invalid. Dimensions::contains then returns false,
    // effectively slicing out this dimensions from the data.
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
} // namespace scipp::core
