// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/variable/except.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/subspan_view.h"

using namespace scipp;

class SubspanViewTest : public ::testing::Test {
protected:
  Variable var{makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3},
                                    sc_units::m, Values{1, 2, 3, 4, 5, 6})};
  Variable var_with_errors{makeVariable<double>(
      Dims{Dim::Y, Dim::X}, Shape{2, 3}, sc_units::m, Values{1, 2, 3, 4, 5, 6},
      Variances{7, 8, 9, 10, 11, 12})};
};

TEST_F(SubspanViewTest, fail_not_inner) {
  EXPECT_THROW([[maybe_unused]] auto view = subspan_view(var, Dim::Y),
               except::DimensionError);
}

TEST_F(SubspanViewTest, values) {
  auto view = subspan_view(var, Dim::X);
  EXPECT_EQ(view.dims(), Dimensions({Dim::Y, 2}));
  EXPECT_EQ(view.unit(), sc_units::m);
  EXPECT_TRUE(equals(view.values<std::span<double>>()[0], {1, 2, 3}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[1], {4, 5, 6}));
  EXPECT_FALSE(view.has_variances());
}

TEST_F(SubspanViewTest, values_length_0) {
  auto view = subspan_view(var.slice({Dim::X, 0, 0}), Dim::X);
  EXPECT_EQ(view.dims(), Dimensions({Dim::Y, 2}));
  EXPECT_EQ(view.unit(), sc_units::m);
  // Note the `const` here: Temporary returned by `slice()` uses `const Variable
  // &` overload.
  EXPECT_TRUE(view.values<std::span<const double>>()[0].empty());
  EXPECT_TRUE(view.values<std::span<const double>>()[1].empty());
  EXPECT_FALSE(view.has_variances());
}

TEST_F(SubspanViewTest, values_and_errors) {
  auto view = subspan_view(var_with_errors, Dim::X);
  EXPECT_EQ(view.dims(), Dimensions({Dim::Y, 2}));
  EXPECT_EQ(view.unit(), sc_units::m);
  EXPECT_TRUE(equals(view.values<std::span<double>>()[0], {1, 2, 3}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[1], {4, 5, 6}));
  EXPECT_TRUE(equals(view.variances<std::span<double>>()[0], {7, 8, 9}));
  EXPECT_TRUE(equals(view.variances<std::span<double>>()[1], {10, 11, 12}));
}

TEST_F(SubspanViewTest, values_and_errors_length_0) {
  auto view = subspan_view(var_with_errors.slice({Dim::X, 0, 0}), Dim::X);
  EXPECT_EQ(view.dims(), Dimensions({Dim::Y, 2}));
  EXPECT_EQ(view.unit(), sc_units::m);
  EXPECT_TRUE(view.values<std::span<const double>>()[0].empty());
  EXPECT_TRUE(view.values<std::span<const double>>()[1].empty());
  EXPECT_TRUE(view.variances<std::span<const double>>()[0].empty());
  EXPECT_TRUE(view.variances<std::span<const double>>()[1].empty());
}

TEST_F(SubspanViewTest, view_of_const) {
  const auto &const_var = var;
  auto view = subspan_view(const_var, Dim::X);
  EXPECT_NO_THROW(view.values<std::span<const double>>());
}

TEST_F(SubspanViewTest, broadcast) {
  const auto &broadcasted = broadcast(var.slice({Dim::Y, 0}), var.dims());
  auto view = subspan_view(broadcasted, Dim::X);
  EXPECT_EQ(view.dims(), Dimensions({Dim::Y, 2}));
  EXPECT_EQ(view.unit(), sc_units::m);
  EXPECT_TRUE(equals(view.values<std::span<const double>>()[0], {1, 2, 3}));
  EXPECT_TRUE(equals(view.values<std::span<const double>>()[1], {1, 2, 3}));
}

TEST_F(SubspanViewTest, broadcast_mutable_fails) {
  auto broadcasted = broadcast(var.slice({Dim::Y, 0}), var.dims());
  // We could in principle return with dtype=std::span<const T> in this case,
  // but in practice this is likely not useful since the caller of subspan_view
  // typically expects that they can modify data.
  EXPECT_THROW_DISCARD(subspan_view(broadcasted, Dim::X),
                       except::VariableError);
}

class SubspanViewOfSliceTest : public ::testing::Test {
protected:
  Variable var{makeVariable<double>(
      Dims{Dim::Z, Dim::Y, Dim::X}, Shape{3, 3, 3}, sc_units::m,
      Values{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
             15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27})};
};

TEST_F(SubspanViewOfSliceTest, inner_slice_left) {
  var = var.slice({Dim::X, 0, 2});
  auto view = subspan_view(var, Dim::X);
  EXPECT_TRUE(equals(view.values<std::span<double>>()[0], {1, 2}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[1], {4, 5}));
}

TEST_F(SubspanViewOfSliceTest, inner_slice_right) {
  var = var.slice({Dim::X, 1, 3});
  auto view = subspan_view(var, Dim::X);
  EXPECT_TRUE(equals(view.values<std::span<double>>()[0], {2, 3}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[1], {5, 6}));
}

TEST_F(SubspanViewOfSliceTest, middle_slice) {
  var = var.slice({Dim::Y, 1, 3});
  auto view = subspan_view(var, Dim::X);
  EXPECT_TRUE(equals(view.values<std::span<double>>()[0], {4, 5, 6}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[1], {7, 8, 9}));
}

TEST_F(SubspanViewOfSliceTest, outer_slice) {
  var = var.slice({Dim::Z, 1, 3});
  auto view = subspan_view(var, Dim::X);
  EXPECT_TRUE(equals(view.values<std::span<double>>()[0], {10, 11, 12}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[1], {13, 14, 15}));
}

TEST_F(SubspanViewOfSliceTest, broadcast) {
  const auto var2 =
      broadcast(var.slice({Dim::Y, 0}), var.dims()).slice({Dim::X, 1, 3});
  auto view = subspan_view(var2, Dim::X);
  EXPECT_TRUE(equals(view.values<std::span<const double>>()[0], {2, 3}));
  EXPECT_TRUE(equals(view.values<std::span<const double>>()[1], {2, 3}));
  EXPECT_TRUE(equals(view.values<std::span<const double>>()[2], {2, 3}));
  EXPECT_TRUE(equals(view.values<std::span<const double>>()[3], {11, 12}));
}

TEST_F(SubspanViewOfSliceTest, transpose) {
  var = var.transpose(std::vector<Dim>{Dim::Y, Dim::Z, Dim::X});
  auto view = subspan_view(var, Dim::X);
  EXPECT_TRUE(equals(view.values<std::span<double>>()[0], {1, 2, 3}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[1], {10, 11, 12}));
}

TEST_F(SubspanViewOfSliceTest, slice_transpose) {
  var = var.transpose(std::vector<Dim>{Dim::Y, Dim::Z, Dim::X});
  for (auto dim : {Dim::X, Dim::Y, Dim::Z})
    var = var.slice({dim, 1, 3});
  auto view = subspan_view(var, Dim::X);
  EXPECT_TRUE(equals(view.values<std::span<double>>()[0], {14, 15}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[1], {23, 24}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[2], {17, 18}));
  EXPECT_TRUE(equals(view.values<std::span<double>>()[3], {26, 27}));
}
