// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "scipp/core/view_index.h"

#include <algorithm>

#include "scipp/core/except.h"

namespace scipp::core {

ViewIndex::ViewIndex(const Dimensions &target_dimensions,
                     const Strides &strides)
    : m_ndim{static_cast<int32_t>(target_dimensions.ndim())} {
  for (scipp::index d = 0; d < m_ndim; ++d)
    m_shape[d] = target_dimensions.size(m_ndim - 1 - d);

  scipp::index rewind = 0;
  for (scipp::index d = 0; d < m_ndim; ++d) {
    m_delta[d] = strides[m_ndim - 1 - d] - rewind;
    rewind = m_shape[d] * strides[m_ndim - 1 - d];
  }
  std::reverse_copy(strides.begin(), strides.begin() + m_ndim,
                    m_strides.begin());
}

} // namespace scipp::core
