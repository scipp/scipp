// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/subspan_view.h"

using namespace scipp;
using namespace scipp::core;

class SubspanViewTest : public ::testing::Test {
protected:
  Variable var{makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, units::m,
                                    {1, 2, 3, 4, 5, 6})};
  Variable var_with_errors{makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}},
                                                units::m, {1, 2, 3, 4, 5, 6},
                                                {7, 8, 9, 10, 11, 12})};
};

TEST_F(SubspanViewTest, fail_sparse) {
  auto sparse = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  EXPECT_THROW(subspan_view(sparse, Dim::X), except::DimensionError);
  EXPECT_THROW(subspan_view(sparse, Dim::Y), except::DimensionError);
}

TEST_F(SubspanViewTest, fail_not_inner) {
  EXPECT_THROW(subspan_view(var, Dim::Y), except::DimensionError);
}

TEST_F(SubspanViewTest, values) {
  auto view = subspan_view(var, Dim::X);
  EXPECT_EQ(view.dims(), Dimensions({Dim::Y, 2}));
  EXPECT_EQ(view.unit(), units::m);
  EXPECT_TRUE(equals(view.values<span<double>>()[0], {1, 2, 3}));
  EXPECT_TRUE(equals(view.values<span<double>>()[1], {4, 5, 6}));
  EXPECT_FALSE(view.hasVariances());
}

TEST_F(SubspanViewTest, values_and_errors) {
  auto view = subspan_view(var_with_errors, Dim::X);
  EXPECT_EQ(view.dims(), Dimensions({Dim::Y, 2}));
  EXPECT_EQ(view.unit(), units::m);
  EXPECT_TRUE(equals(view.values<span<double>>()[0], {1, 2, 3}));
  EXPECT_TRUE(equals(view.values<span<double>>()[1], {4, 5, 6}));
  EXPECT_TRUE(equals(view.variances<span<double>>()[0], {7, 8, 9}));
  EXPECT_TRUE(equals(view.variances<span<double>>()[1], {10, 11, 12}));
}

TEST_F(SubspanViewTest, view_of_const) {
  const auto &const_var = var;
  auto view = subspan_view(const_var, Dim::X);
  EXPECT_NO_THROW(view.values<span<const double>>());
}
