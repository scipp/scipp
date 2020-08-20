// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"

namespace scipp::core {

namespace {
void expectCanBroadcastFromTo(const Dimensions &source,
                              const Dimensions &target) {
  if (source == target)
    return;
  for (const auto dim : target.labels())
    if (source.contains(dim) && (source[dim] < target[dim]))
      throw except::DimensionError("Cannot broadcast/slice dimension since "
                                   "data has mismatching but smaller "
                                   "dimension extent.");
}
} // namespace

/// Construct an element_array_view.
///
/// @param offset Start offset from beginning of data array.
/// @param targetDimensions Dimensions of the constructed ElementArrayView.
/// @param dimensions Dimensions of the data array.
///
/// The parameter `targetDimensions` can be used to remove, slice, broadcast,
/// or transpose dimensions of the input data array.
element_array_view::element_array_view(const scipp::index offset,
                                       const Dimensions &targetDimensions,
                                       const Dimensions &dimensions)
    : m_offset(offset), m_targetDimensions(targetDimensions),
      m_dimensions(dimensions) {
  expectCanBroadcastFromTo(m_dimensions, m_targetDimensions);
}

/// Construct element_array_view from another element_array_view, with
/// different target dimensions.
///
/// A good way to think of this is of a non-contiguous underlying data array,
/// e.g., since the other view may represent a slice. This also supports
/// broadcasting the slice.
element_array_view::element_array_view(const element_array_view &other,
                                       const Dimensions &targetDimensions)
    : m_offset(other.m_offset), m_targetDimensions(targetDimensions) {
  expectCanBroadcastFromTo(other.m_targetDimensions, m_targetDimensions);
  m_dimensions = other.m_dimensions;
  // See implementation of ViewIndex regarding this relabeling.
  for (const auto label : m_dimensions.labels())
    if (label != Dim::Invalid && !other.m_targetDimensions.contains(label))
      m_dimensions.relabel(m_dimensions.index(label), Dim::Invalid);
}

} // namespace scipp::core
