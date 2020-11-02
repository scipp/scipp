// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/buckets.h"
#include "scipp/variable/operations.h"

using namespace scipp;

class VariableBucketTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  Variable var = from_constituents(indices, Dim::X, buffer);
};

TEST_F(VariableBucketTest, comparison) {
  EXPECT_TRUE(var == var);
  EXPECT_FALSE(var != var);
}

TEST_F(VariableBucketTest, copy) { EXPECT_EQ(Variable(var), var); }

TEST_F(VariableBucketTest, assign) {
  Variable copy(var);
  var.values<bucket<Variable>>()[0] += var.values<bucket<Variable>>()[1];
  EXPECT_NE(copy, var);
  copy = var;
  EXPECT_EQ(copy, var);
}

TEST_F(VariableBucketTest, copy_view) {
  EXPECT_EQ(Variable(var.slice({Dim::Y, 0, 2})), var);
  EXPECT_EQ(Variable(var.slice({Dim::Y, 0, 1})), var.slice({Dim::Y, 0, 1}));
  EXPECT_EQ(Variable(var.slice({Dim::Y, 1, 2})), var.slice({Dim::Y, 1, 2}));
}

TEST_F(VariableBucketTest, shape_operations) {
  // Not supported yet, not to ensure this fails instead of returning garbage.
  EXPECT_ANY_THROW(concatenate(var, var, Dim::Y));
}

TEST_F(VariableBucketTest, basics) {
  // TODO Probably it would be a good idea to prevent having any other unit.
  // Does this imply unit should move from Variable into VariableConcept?
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

TEST_F(VariableBucketTest, view) {
  VariableView view(var);
  EXPECT_EQ(view.values<bucket<Variable>>(), var.values<bucket<Variable>>());
  view = var.slice({Dim::Y, 1});
  const auto vals = view.values<bucket<Variable>>();
  EXPECT_EQ(vals.size(), 1);
  EXPECT_EQ(vals[0], buffer.slice({Dim::X, 2, 4}));
}

TEST_F(VariableBucketTest, construct_from_view) {
  Variable copy{VariableView(var)};
  EXPECT_EQ(copy, var);
}

TEST_F(VariableBucketTest, unary_operation) {
  const auto expected = from_constituents(indices, Dim::X, sqrt(buffer));
  EXPECT_EQ(sqrt(var), expected);
  EXPECT_EQ(sqrt(var.slice({Dim::Y, 1})), expected.slice({Dim::Y, 1}));
}

TEST_F(VariableBucketTest, binary_operation) {
  const auto expected = from_constituents(indices, Dim::X, buffer + buffer);
  EXPECT_EQ(var + var, expected);
  EXPECT_EQ(var.slice({Dim::Y, 1}) + var.slice({Dim::Y, 1}),
            expected.slice({Dim::Y, 1}));
}

TEST_F(VariableBucketTest, binary_operation_with_dense) {
  Variable dense = makeVariable<double>(var.dims(), Values{0.1, 0.2});
  Variable expected_buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1.1, 2.1, 3.2, 4.2});
  const auto expected = from_constituents(indices, Dim::X, expected_buffer);
  EXPECT_EQ(var + dense, expected);
  EXPECT_EQ(var.slice({Dim::Y, 1}) + dense.slice({Dim::Y, 1}),
            expected.slice({Dim::Y, 1}));
}

TEST_F(VariableBucketTest, binary_operation_with_dense_broadcast) {
  Variable dense =
      makeVariable<double>(Dims{Dim::Z}, Shape{2}, Values{0.1, 0.2});
  Variable expected_buffer = makeVariable<double>(
      Dims{Dim::X}, Shape{8}, Values{1.1, 2.1, 1.2, 2.2, 3.1, 4.1, 3.2, 4.2});
  Variable expected_indices =
      makeVariable<std::pair<scipp::index, scipp::index>>(
          Dims{Dim::Y, Dim::Z}, Shape{2, 2},
          Values{std::pair{0, 2}, std::pair{2, 4}, std::pair{4, 6},
                 std::pair{6, 8}});
  const auto expected =
      from_constituents(expected_indices, Dim::X, expected_buffer);
  EXPECT_EQ(var + dense, expected);
  EXPECT_EQ(var.slice({Dim::Y, 1}) + dense, expected.slice({Dim::Y, 1}));
  EXPECT_EQ(dense + var, transpose(expected));
}

TEST_F(VariableBucketTest, to_constituents) {
  auto [idx0, dim0, buf0] = VariableView(var).constituents<bucket<Variable>>();
  auto idx_ptr = idx0.values<std::pair<scipp::index, scipp::index>>().data();
  auto buf_ptr = buf0.values<double>().data();
  auto [idx1, dim1, buf1] = var.to_constituents<bucket<Variable>>();
  EXPECT_FALSE(var);
  EXPECT_EQ((idx1.values<std::pair<scipp::index, scipp::index>>().data()),
            idx_ptr);
  EXPECT_EQ(buf1.values<double>().data(), buf_ptr);
  EXPECT_EQ(idx1, indices);
  EXPECT_EQ(dim1, Dim::X);
  EXPECT_EQ(buf1, buffer);
}
