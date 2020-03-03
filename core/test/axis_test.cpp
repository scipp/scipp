// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/axis.h"

using namespace scipp;
using namespace scipp::core;

class DataArrayAxis_comparison_operators : public ::testing::Test {
protected:
  DataArrayAxis_comparison_operators()
      : var1(makeVariable<double>(Values{1})),
        var2(makeVariable<double>(Values{2})) {}
  void expect_eq(const DataArrayAxisConstView &a,
                 const DataArrayAxisConstView &b) const {
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);
  }
  void expect_ne(const DataArrayAxisConstView &a,
                 const DataArrayAxisConstView &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
  }

  Variable var1;
  Variable var2;
};

TEST_F(DataArrayAxis_comparison_operators, data_only) {
  DataArrayAxis a(var1);
  expect_eq(a, a);
  expect_eq(a, DataArrayAxis(var1));
  expect_ne(a, DataArrayAxis());
  expect_ne(a, DataArrayAxis(var2));
  expect_ne(a, DataArrayAxis(var1, var2));
  expect_ne(a, DataArrayAxis(Variable{}, var2));
}

TEST_F(DataArrayAxis_comparison_operators, unaligned_only) {
  DataArrayAxis a(Variable{}, var2);
  expect_eq(a, a);
  expect_eq(a, DataArrayAxis(Variable{}, var2));
  expect_ne(a, DataArrayAxis());
  expect_ne(a, DataArrayAxis(var2));
  expect_ne(a, DataArrayAxis(var1, var2));
  expect_ne(a, DataArrayAxis(Variable{}, var1));
}

TEST_F(DataArrayAxis_comparison_operators, data_and_unaligned) {
  DataArrayAxis a(var1, var2);
  expect_eq(a, a);
  expect_eq(a, DataArrayAxis(var1, var2));
  expect_ne(a, DataArrayAxis());
  expect_ne(a, DataArrayAxis(var1));
  expect_ne(a, DataArrayAxis(Variable{}, var2));
  expect_ne(a, DataArrayAxis(var1, var1));
  expect_ne(a, DataArrayAxis(var2, var2));
}

class DatasetAxis_comparison_operators : public ::testing::Test {
protected:
  DatasetAxis_comparison_operators()
      : var1(makeVariable<double>(Values{1})),
        var2(makeVariable<double>(Values{2})),
        var3(makeVariable<double>(Values{3})) {}
  void expect_eq(const DatasetAxisConstView &a,
                 const DatasetAxisConstView &b) const {
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);
  }
  void expect_ne(const DatasetAxisConstView &a,
                 const DatasetAxisConstView &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
  }

  Variable var1;
  Variable var2;
  Variable var3;
};

TEST_F(DatasetAxis_comparison_operators, data_only) {
  DatasetAxis a(var1);
  expect_eq(a, a);
  expect_eq(a, DatasetAxis(var1));
  expect_ne(a, DatasetAxis());
  expect_ne(a, DatasetAxis(var2));
  expect_ne(a, DatasetAxis(var1, {{"a", var2}}));
  expect_ne(a, DatasetAxis(Variable{}, {{"a", var2}}));
}

TEST_F(DatasetAxis_comparison_operators, unaligned_only) {
  DatasetAxis a(Variable{}, {{"a", var2}});
  expect_eq(a, a);
  expect_eq(a, DatasetAxis(Variable{}, {{"a", var2}}));
  expect_ne(a, DatasetAxis());
  expect_ne(a, DatasetAxis(var2));
  expect_ne(a, DatasetAxis(var1, {{"a", var2}}));
  expect_ne(a, DatasetAxis(Variable{}, {{"a", var1}}));
  expect_ne(a, DatasetAxis(Variable{}, {{"b", var2}}));
  expect_ne(a, DatasetAxis(Variable{}, {{"a", var2}, {"b", var2}}));
}

TEST_F(DatasetAxis_comparison_operators, data_and_unaligned) {
  DatasetAxis a(var1, {{"a", var2}});
  expect_eq(a, a);
  expect_eq(a, DatasetAxis(var1, {{"a", var2}}));
  expect_ne(a, DatasetAxis());
  expect_ne(a, DatasetAxis(var1));

  expect_ne(a, DatasetAxis(Variable{}, {{"a", var2}}));
  expect_ne(a, DatasetAxis(var2, {{"a", var2}}));

  expect_ne(a, DatasetAxis(var1, {{"a", var1}}));
  expect_ne(a, DatasetAxis(var1, {{"b", var2}}));
  expect_ne(a, DatasetAxis(var1, {{"a", var2}, {"b", var2}}));
}

TEST(DataArrayAxisTest, construct_default) {
  DataArrayAxis axis;
  EXPECT_FALSE(axis.hasData());
  EXPECT_FALSE(axis.hasUnaligned());
  EXPECT_FALSE(axis.unaligned());
}

TEST(DatasetAxisTest, construct_default) {
  DatasetAxis axis;
  EXPECT_FALSE(axis.hasData());
  EXPECT_FALSE(axis.hasUnaligned());
  EXPECT_TRUE(axis.unaligned().empty());
}

class AxisTest : public ::testing::Test {
protected:
  AxisTest()
      : var1(makeVariable<double>(Values{1})),
        var2(makeVariable<double>(Values{2})),
        var3(makeVariable<double>(Values{3})), axis_a(var1, var2),
        axis_b(var1, var3), axis(var1, {{"a", var2}, {"b", var3}})
  {}

  Variable var1;
  Variable var2;
  Variable var3;
  DataArrayAxis axis_a;
  DataArrayAxis axis_b;
  DatasetAxis axis;
};

TEST_F(AxisTest, fixture) {
  EXPECT_NE(var1, var2);
  EXPECT_NE(var1, var3);
  EXPECT_NE(var2, var3);
  EXPECT_NE(axis_a, axis_b);
}

TEST_F(AxisTest, DataArrayAxis_construct_from_view) {
  DataArrayAxisConstView const_view(axis_a);
  DataArrayAxis copy(const_view);
  EXPECT_EQ(copy, axis_a);
}

TEST_F(AxisTest, DatasetAxis_construct_from_view) {
  DatasetAxisConstView const_view(axis);
  DatasetAxis copy(const_view);
  EXPECT_EQ(copy, axis);
}

TEST_F(AxisTest, DataArrayAxis_construct_from_dataset_view) {
  EXPECT_EQ(DataArrayAxis(axis["a"]), axis_a);
  EXPECT_EQ(DataArrayAxis(axis["b"]), axis_b);
}

TEST_F(AxisTest, to_DatasetAxis) {
  const auto ax = DataArrayAxis::to_DatasetAxis(DataArrayAxis(axis_a), "c");
  EXPECT_EQ(ax["c"], axis_a);
}

TEST(DatasetAxisTest, hasUnaligned) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{4});
  DatasetAxis axis(var);
  EXPECT_FALSE(axis.hasUnaligned());
  EXPECT_FALSE(axis["a"].hasUnaligned());
  EXPECT_FALSE(DatasetAxisConstView(axis).hasUnaligned());
  axis.unaligned().set("a", var);
  EXPECT_TRUE(axis.hasUnaligned());
  EXPECT_TRUE(axis["a"].hasUnaligned());
  EXPECT_FALSE(axis["b"].hasUnaligned());
  EXPECT_TRUE(DatasetAxisConstView(axis).hasUnaligned());
}

TEST(DatasetAxisTest, concatenate) {
  const auto var1 = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  const auto var2 = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{3});
  DatasetAxis axis1(var1);
  DatasetAxis axis2(var2);

  DatasetAxis expected(concatenate(var1, var2, Dim::X));
  EXPECT_EQ(concatenate(axis1, axis2, Dim::X), expected);
}
