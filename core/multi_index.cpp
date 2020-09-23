// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/multi_index.h"
#include "scipp/core/except.h"

namespace scipp::core {
void validate_bucket_indices_impl(const element_array_view &param0,
                                  const element_array_view &param1) {
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
