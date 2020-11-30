// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/multi_index.h"
#include "scipp/core/except.h"

namespace scipp::core {

/// Strides in dataDims when iterating iterDims.
std::array<scipp::index, NDIM_MAX> get_strides(const Dimensions &iterDims,
                                               const Dimensions &dataDims) {
  std::array<scipp::index, NDIM_MAX> strides = {};
  scipp::index d = iterDims.ndim() - 1;
  for (const auto dim : iterDims.labels()) {
    if (dataDims.contains(dim))
      strides[d--] = dataDims.offset(dim);
    else
      strides[d--] = 0;
  }
  return strides;
}

void validate_bucket_indices_impl(const ElementArrayViewParams &param0,
                                  const ElementArrayViewParams &param1) {
  const auto iterDims = param0.dims();
  auto index = MultiIndex(iterDims, param0.dataDims(), param1.dataDims());
  const auto indices0 = param0.bucketParams().indices;
  const auto indices1 = param1.bucketParams().indices;
  constexpr auto size = [](const auto range) {
    return range.second - range.first;
  };
  for (scipp::index i = 0; i < iterDims.volume(); ++i) {
    const auto [i0, i1] = index.get();
    if (size(indices0[i0]) != size(indices1[i1]))
      throw except::BucketError("Bucket size mismatch");
    index.increment();
  }
}

} // namespace scipp::core
