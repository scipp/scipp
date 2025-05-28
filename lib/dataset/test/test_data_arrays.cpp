// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"

#include "test_data_arrays.h"

using namespace scipp;

DataArray make_data_array_1d(const int64_t seed) {
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::counts,
                                   Values{seed + 1, seed + 2},
                                   Variances{seed + 3, seed + 4});
  auto coord =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m, Values{1, 2});
  auto mask =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{seed % 2 == 0, false});
  const auto name = std::to_string(seed);
  auto scalar_coord = makeVariable<int64_t>(Values{12});
  auto scalar_mask = makeVariable<bool>(Values{false});
  return DataArray(
      data, {{Dim::X, coord}, {Dim("scalar"), scalar_coord}},
      {{"mask", mask}, {"mask" + name, mask}, {"scalar_mask", scalar_mask}},
      "array" + name);
}
