// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include "scipp/variable/variable.h"

using namespace scipp;

inline Variable arange(const Dim dim, const scipp::index shape) {
  std::vector<double> values(shape);
  for (scipp::index i = 0; i < shape; ++i)
    values[i] = i;
  return makeVariable<double>(Dims{dim}, Shape{shape}, Values(values));
}

/**
 * Convert an array of sizes into index pairs for partitions of the given sizes.
 * Example:
 *  index_pairs_from_int_partition({2, 4, 0, 1})
 *  -> {<0,2>, <2,6>, <6,6>, <6,7>}
 */
inline auto index_pairs_from_sizes(const std::vector<scipp::index> &partition) {
  std::vector<scipp::index_pair> index_pairs;
  index_pairs.reserve(partition.size());
  std::transform(partition.begin(), partition.end(),
                 std::back_inserter(index_pairs),
                 [lower = 0](const auto n) mutable {
                   const auto upper = lower + n;
                   return scipp::index_pair{std::exchange(lower, upper), upper};
                 });
  return index_pairs;
}