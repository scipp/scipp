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
/// @param offset Start offset from beginning of array.
/// @param iterDims Dimensions to use for iteration.
/// @param dataDims Dimensions of array to iterate.
///
/// The parameter `iterDims` can be used to remove, slice, broadcast, or
/// transpose `dataDims`.
element_array_view::element_array_view(const scipp::index offset,
                                       const Dimensions &iterDims,
                                       const Dimensions &dataDims)
    : m_offset(offset), m_iterDims(iterDims), m_dataDims(dataDims) {
  expectCanBroadcastFromTo(m_dataDims, m_iterDims);
}

/// Construct element_array_view from another element_array_view, with
/// different iteration dimensions.
///
/// A good way to think of this is of a non-contiguous underlying data array,
/// e.g., since the other view may represent a slice. This also supports
/// broadcasting the slice.
element_array_view::element_array_view(const element_array_view &other,
                                       const Dimensions &iterDims)
    : m_offset(other.m_offset), m_iterDims(iterDims),
      m_dataDims(other.m_dataDims), m_bucketParams(other.m_bucketParams) {
  expectCanBroadcastFromTo(other.m_iterDims, m_iterDims);
  // See implementation of ViewIndex regarding this relabeling.
  for (const auto label : m_dataDims.labels())
    if (label != Dim::Invalid && !other.m_iterDims.contains(label))
      m_dataDims.relabel(m_dataDims.index(label), Dim::Invalid);
}

} // namespace scipp::core
