// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/variable/reduction.h"
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

class SqueezeTest : public ::testing::Test {
protected:
  Variable var = makeVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z},
                                      Shape{1, 2, 1}, Values{1, 2});
  Variable original = var;
};

TEST_F(SqueezeTest, fail) {
  EXPECT_THROW(squeeze(var, {Dim::Y}), except::DimensionError);
  EXPECT_EQ(var, original);
  EXPECT_THROW(squeeze(var, {Dim::X, Dim::Y}), except::DimensionError);
  EXPECT_EQ(var, original);
  EXPECT_THROW(squeeze(var, {Dim::Y, Dim::Z}), except::DimensionError);
  EXPECT_EQ(var, original);
}

TEST_F(SqueezeTest, none) {
  squeeze(var, {});
  EXPECT_EQ(var, original);
}

TEST_F(SqueezeTest, outer) {
  squeeze(var, {Dim::X});
  EXPECT_EQ(var, sum(original, Dim::X));
}

TEST_F(SqueezeTest, inner) {
  squeeze(var, {Dim::Z});
  EXPECT_EQ(var, sum(original, Dim::Z));
}

TEST_F(SqueezeTest, both) {
  squeeze(var, {Dim::X, Dim::Z});
  EXPECT_EQ(var, sum(sum(original, Dim::Z), Dim::X));
}
