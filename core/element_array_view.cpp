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

/// Construct ElementArrayViewParams.
///
/// @param offset Start offset from beginning of array.
/// @param iterDims Dimensions to use for iteration.
/// @param dataDims Dimensions of array to iterate.
/// @param bucketParams Optional, in case of view onto bucket-variable this
/// holds parameters for accessing individual buckets.
///
/// The parameter `iterDims` can be used to remove, slice, broadcast, or
/// transpose `dataDims`.
ElementArrayViewParams::ElementArrayViewParams(const scipp::index offset,
                                               const Dimensions &iterDims,
                                               const Dimensions &dataDims,
                                               const BucketParams &bucketParams)
    : m_offset(offset), m_iterDims(iterDims), m_dataDims(dataDims),
      m_bucketParams(bucketParams) {
  expectCanBroadcastFromTo(m_dataDims, m_iterDims);
}

/// Construct ElementArrayViewParams from another ElementArrayViewParams, with
/// different iteration dimensions.
///
/// A good way to think of this is of a non-contiguous underlying data array,
/// e.g., since the other view may represent a slice. This also supports
/// broadcasting the slice.
ElementArrayViewParams::ElementArrayViewParams(
    const ElementArrayViewParams &other, const Dimensions &iterDims)
    : m_offset(other.m_offset), m_iterDims(iterDims),
      m_dataDims(other.m_dataDims), m_bucketParams(other.m_bucketParams) {
  expectCanBroadcastFromTo(other.m_iterDims, m_iterDims);
  // See implementation of ViewIndex regarding this relabeling.
  for (const auto label : m_dataDims.labels())
    if (label != Dim::Invalid && !other.m_iterDims.contains(label))
      m_dataDims.relabel(m_dataDims.index(label), Dim::Invalid);
}

void ElementArrayViewParams::requireContiguous() const {
  if (m_iterDims != m_dataDims || m_bucketParams)
    throw std::runtime_error("Data is not contiguous");
}

} // namespace scipp::core
