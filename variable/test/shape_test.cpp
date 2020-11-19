// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/except.h"
#include "scipp/variable/shape.h"

using namespace scipp;

TEST(ShapeTest, broadcast) {
  auto reference =
      makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X}, Shape{3, 2, 2},
                           Values{1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4},
                           Variances{5, 6, 7, 8, 5, 6, 7, 8, 5, 6, 7, 8});
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});

  // No change if dimensions exist.
  EXPECT_EQ(broadcast(var, {Dim::X, 2}), var);
  EXPECT_EQ(broadcast(var, {Dim::Y, 2}), var);
  EXPECT_EQ(broadcast(var, {{Dim::Y, 2}, {Dim::X, 2}}), var);

  // No transpose done, should this fail? Failing is not really necessary since
  // we have labeled dimensions.
  EXPECT_EQ(broadcast(var, {{Dim::X, 2}, {Dim::Y, 2}}), var);

  EXPECT_EQ(broadcast(var, {Dim::Z, 3}), reference);
}

TEST(ShapeTest, broadcast_fail) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1, 2, 3, 4});
  EXPECT_THROW_MSG(broadcast(var, {Dim::X, 3}), except::DimensionLengthError,
                   "Expected dimension to be in {{y, 2}, {x, 2}}, "
                   "got x with mismatching length 3.");
}
