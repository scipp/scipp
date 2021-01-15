// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

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

TEST(VariableTest, reshape) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                        units::m, Values{1, 2, 3, 4, 5, 6});

  ASSERT_EQ(reshape(var, {Dim::Row, 6}),
            makeVariable<double>(Dims{Dim::Row}, Shape{6}, units::m,
                                 Values{1, 2, 3, 4, 5, 6}));
  ASSERT_EQ(reshape(var, {{Dim::Row, 3}, {Dim::Z, 2}}),
            makeVariable<double>(Dims{Dim::Row, Dim::Z}, Shape{3, 2}, units::m,
                                 Values{1, 2, 3, 4, 5, 6}));
}

TEST(VariableTest, reshape_with_variance) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                        Values{1, 2, 3, 4, 5, 6},
                                        Variances{7, 8, 9, 10, 11, 12});

  ASSERT_EQ(reshape(var, {Dim::Row, 6}),
            makeVariable<double>(Dims{Dim::Row}, Shape{6},
                                 Values{1, 2, 3, 4, 5, 6},
                                 Variances{7, 8, 9, 10, 11, 12}));
  ASSERT_EQ(reshape(var, {{Dim::Row, 3}, {Dim::Z, 2}}),
            makeVariable<double>(Dims{Dim::Row, Dim::Z}, Shape{3, 2},
                                 Values{1, 2, 3, 4, 5, 6},
                                 Variances{7, 8, 9, 10, 11, 12}));
}

TEST(VariableTest, reshape_temporary) {
  const auto var = makeVariable<double>(
      Dims{Dim::X, Dim::Row}, Shape{2, 4}, units::m,
      Values{1, 2, 3, 4, 5, 6, 7, 8}, Variances{9, 10, 11, 12, 13, 14, 15, 16});
  auto reshaped = reshape(sum(var, Dim::X), {{Dim::Y, 2}, {Dim::Z, 2}});
  ASSERT_EQ(reshaped, makeVariable<double>(Dims{Dim::Y, Dim::Z}, Shape{2, 2},
                                           units::m, Values{6, 8, 10, 12},
                                           Variances{22, 24, 26, 28}));

  EXPECT_EQ(typeid(decltype(reshape(std::move(var), {}))), typeid(Variable));
}

TEST(VariableTest, reshape_fail) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                  Values{1, 2, 3, 4, 5, 6});
  EXPECT_THROW_MSG(reshape(var, {Dim::Row, 5}), std::runtime_error,
                   "Cannot reshape to dimensions with different volume");
  EXPECT_THROW_MSG(reshape(var.slice({Dim::X, 1}), {Dim::Row, 5}),
                   std::runtime_error,
                   "Cannot reshape to dimensions with different volume");
}

TEST(VariableTest, reshape_and_slice) {
  auto var = makeVariable<double>(
      Dims{Dim::Z}, Shape{16},
      Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
      Variances{17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
                32});

  auto slice = reshape(var, {{Dim::X, 4}, {Dim::Y, 4}})
                   .slice({Dim::X, 1, 3})
                   .slice({Dim::Y, 1, 3});
  ASSERT_EQ(slice, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                        Values{6, 7, 10, 11},
                                        Variances{22, 23, 26, 27}));

  Variable center = reshape(reshape(var, {{Dim::X, 4}, {Dim::Y, 4}})
                                .slice({Dim::X, 1, 3})
                                .slice({Dim::Y, 1, 3}),
                            {Dim::Z, 4});

  ASSERT_EQ(center,
            makeVariable<double>(Dims{Dim::Z}, Shape{4}, Values{6, 7, 10, 11},
                                 Variances{22, 23, 26, 27}));
}

TEST(VariableTest, reshape_mutable) {
  auto modified_original = makeVariable<double>(
      Dims{Dim::X, Dim::Y}, Shape{2, 3}, Values{1, 2, 3, 0, 5, 6});
  auto reference =
      makeVariable<double>(Dims{Dim::Row}, Shape{6}, Values{1, 2, 3, 0, 5, 6});

  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                  Values{1, 2, 3, 4, 5, 6});

  auto view = reshape(var, {Dim::Row, 6});
  view.values<double>()[3] = 0;

  ASSERT_EQ(view, reference);
  ASSERT_EQ(var, modified_original);
}
