// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
  for (const auto &dim : target.labels())
    if (source.contains(dim) && (source[dim] < target[dim]))
      throw except::DimensionError("Cannot broadcast/slice dimension since "
                                   "data has mismatching but smaller "
                                   "dimension extent.");
}
} // namespace

/// Construct ElementArrayViewParams.
///
/// @param offset Start offset from beginning of array.
/// @param iter_dims Dimensions to use for iteration.
/// @param strides Strides in memory, order matches that of iterDims.
/// @param bucket_params Optional, in case of view onto bucket-variable this
/// holds parameters for accessing individual buckets.
ElementArrayViewParams::ElementArrayViewParams(
    const scipp::index offset, const Dimensions &iter_dims,
    const Strides &strides, const BucketParams &bucket_params)
    : m_offset(offset), m_iterDims(iter_dims), m_strides(strides),
      m_bucketParams(bucket_params) {}

/// Construct ElementArrayViewParams from another ElementArrayViewParams, with
/// different iteration dimensions.
///
/// A good way to think of this is of a non-contiguous underlying data array,
/// e.g., since the other view may represent a slice. This also supports
/// broadcasting the slice.
ElementArrayViewParams::ElementArrayViewParams(
    const ElementArrayViewParams &other, const Dimensions &iterDims)
    : m_offset(other.m_offset), m_iterDims(iterDims),
      m_bucketParams(other.m_bucketParams) {
  expectCanBroadcastFromTo(other.m_iterDims, m_iterDims);

  m_strides.resize(iterDims.ndim());
  for (scipp::index dim = 0; dim < iterDims.ndim(); ++dim) {
    auto label = iterDims.label(dim);
    if (other.m_iterDims.contains(label)) {
      m_strides[dim] = other.m_strides[other.m_iterDims.index(label)];
    } else {
      m_strides[dim] = 0;
    }
  }
}

void ElementArrayViewParams::requireContiguous() const {
  if (m_bucketParams || m_strides != Strides(m_iterDims))
    throw std::runtime_error("Data is not contiguous");
}

[[nodiscard]] bool
ElementArrayViewParams::overlaps(const ElementArrayViewParams &other) const {
  if (m_offset == other.m_offset && m_iterDims == other.m_iterDims &&
      m_strides == other.m_strides) {
    // When both views are exactly the same, we should be fine without
    // making extra copies.
    return false;
  }
  // Otherwise check for partial overlap.
  const auto [this_begin, this_end] = memory_bounds(
      m_iterDims.shape().begin(), m_iterDims.shape().end(), m_strides.begin());
  const auto [other_begin, other_end] =
      memory_bounds(other.m_iterDims.shape().begin(),
                    other.m_iterDims.shape().end(), other.m_strides.begin());
  return ((this_begin < other_end) && (this_end > other_begin));
}

} // namespace scipp::core
