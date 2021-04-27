// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/core/strides.h"

using namespace scipp;
using namespace scipp::core;

namespace {
void check_strides(const Dimensions &iter, const Dimensions &data,
                   const std::vector<scipp::index> &expected) {
  EXPECT_EQ(Strides(iter, data), Strides(expected));
}
} // namespace

TEST(StridesTest, construct_from_two_dims_full) {
  check_strides({Dim::X, 1}, {Dim::X, 1}, {1});
  check_strides({Dim::X, 2}, {Dim::X, 2}, {1});
}

TEST(StridesTest, construct_from_two_dims_sliced) {
  // Y sliced out, broadcast slice to X
  check_strides({Dim::X, 2}, {Dim::Y, 2}, {0});
}

TEST(StridesTest, construct_from_two_dims_2d) {
  Dimensions yx{{Dim::Y, Dim::X}, {3, 2}};
  Dimensions xy{{Dim::X, Dim::Y}, {2, 3}};
  // full range
  check_strides(yx, yx, {2, 1});
  // transposed
  check_strides(xy, yx, {1, 2});
}
