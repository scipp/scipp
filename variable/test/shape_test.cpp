// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

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
  EXPECT_EQ(broadcast(var, var.dims()), var);
  EXPECT_EQ(broadcast(var, transpose(var.dims())), transpose(var));
  const Dimensions z(Dim::Z, 3);
  EXPECT_EQ(broadcast(var, merge(z, var.dims())), reference);
  EXPECT_EQ(broadcast(var, merge(var.dims(), z)),
            transpose(reference, {Dim::Y, Dim::X, Dim::Z}));
}

TEST(ShapeTest, broadcast_fail) {
  auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                  Values{1, 2, 3, 4});
  EXPECT_THROW(broadcast(var, {Dim::X, 3}), except::NotFoundError);
}
