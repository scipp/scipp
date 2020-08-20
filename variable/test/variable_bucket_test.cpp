// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bucket_model.h"

using namespace scipp;

using Model = variable::DataModel<bucket<Variable>>;

class VariableBucketTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::X, 2};
  element_array<std::pair<scipp::index, scipp::index>> buckets{{0, 2}, {2, 4}};
  Variable buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
};

TEST_F(VariableBucketTest, basics) {
  Variable var(std::make_unique<Model>(dims, buckets, Dim::X, buffer));
  EXPECT_EQ(var.unit(), units::one);
  EXPECT_EQ(var.dims(), dims);
  const auto vals = var.values<bucket<Variable>>();
  EXPECT_EQ(vals.size(), 2);
  EXPECT_EQ(vals[0], buffer.slice({Dim::X, 0, 2}));
  EXPECT_EQ(vals[1], buffer.slice({Dim::X, 2, 4}));
  EXPECT_EQ(vals.front(), buffer.slice({Dim::X, 0, 2}));
  EXPECT_EQ(vals.back(), buffer.slice({Dim::X, 2, 4}));
  EXPECT_EQ(*vals.begin(), buffer.slice({Dim::X, 0, 2}));
  const auto &const_var = var;
  EXPECT_EQ(const_var.values<bucket<Variable>>()[0],
            buffer.slice({Dim::X, 0, 2}));
}
