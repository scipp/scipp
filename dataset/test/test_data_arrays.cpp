// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <gtest/gtest.h>

#include "test_data_arrays.h"

using namespace scipp;

DataArray make_data_array_1d() {
  auto data = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                   Values{1, 2}, Variances{3, 4});
  auto coord =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{1, 2});
  auto mask = makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false});
  auto scalar_coord = makeVariable<int64_t>(Values{12});
  auto scalar_mask = makeVariable<bool>(Values{false});
  return DataArray(data, {{Dim::X, coord}, {Dim("scalar"), scalar_coord}},
                   {{"mask", mask}, {"scalar_mask", scalar_mask}},
                   {{Dim("attr"), coord + coord},
                    {Dim("scalar_attr"), scalar_coord + scalar_coord}});
}
