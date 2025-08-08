// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

#include "scipp/core/except.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/variable.h"

using namespace scipp;

TEST(ReduceTest, min_max_fails) {
  const auto bad = makeVariable<double>(Dims{Dim::X}, Shape{2});
  EXPECT_THROW_DISCARD(min(bad, Dim::Y), except::DimensionError);
  EXPECT_THROW_DISCARD(max(bad, Dim::Y), except::DimensionError);
}

TEST(ReduceTest, min_max) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                        Values{1, 2, 3, 4});
  EXPECT_EQ(max(var, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{3, 4}));
  EXPECT_EQ(max(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4}));
  EXPECT_EQ(min(var, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 2}));
  EXPECT_EQ(min(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 3}));
}

TEST(ReduceTest, min_max_with_variances) {
  const auto var =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                           Values{1, 2, 3, 4}, Variances{5, 6, 7, 8});
  EXPECT_EQ(max(var, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{3, 4},
                                 Variances{7, 8}));
  EXPECT_EQ(max(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{2, 4},
                                 Variances{6, 8}));
  EXPECT_EQ(min(var, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 2},
                                 Variances{5, 6}));
  EXPECT_EQ(min(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 3},
                                 Variances{5, 7}));
}

TEST(ReduceTest, min_max_empty_dim) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 0},
                                        Values{}, Variances{});
  EXPECT_EQ(max(var, Dim::X), makeVariable<double>(Dims{Dim::Y}, Shape{0},
                                                   Values{}, Variances{}));
  const auto highest = std::numeric_limits<double>::max();
  EXPECT_EQ(max(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2},
                                 Values{-highest, -highest}, Variances{0, 0}));
  EXPECT_EQ(min(var, Dim::X), makeVariable<double>(Dims{Dim::Y}, Shape{0},
                                                   Values{}, Variances{}));
  EXPECT_EQ(min(var, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2},
                                 Values{highest, highest}, Variances{0, 0}));
}

TEST(ReduceTest, min_max_all_dims) {
  const auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                        Values{1, 2, 3, 4});
  EXPECT_EQ(min(var), makeVariable<double>(Values{1}));
  EXPECT_EQ(max(var), makeVariable<double>(Values{4}));
  EXPECT_EQ(min(min(var)), min(var));
  EXPECT_EQ(max(min(var)), min(var));
}

TEST(ReduceTest, all_any_all_dims) {
  const auto var = makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                      Values{true, false, false, false});
  EXPECT_EQ(all(var), makeVariable<bool>(Values{false}));
  EXPECT_EQ(any(var), makeVariable<bool>(Values{true}));
  EXPECT_EQ(all(all(var)), all(var));
  EXPECT_EQ(any(all(var)), all(var));
  EXPECT_EQ(all(any(var)), any(var));
  EXPECT_EQ(any(any(var)), any(var));
}

using NansumTypes = ::testing::Types<int32_t, int64_t, float, double>;
template <typename T> struct NansumTest : public ::testing::Test {};
TYPED_TEST_SUITE(NansumTest, NansumTypes);

TYPED_TEST(NansumTest, nansum_all_dims) {
  auto x = makeVariable<TypeParam>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                   Values{1, 1, 2, 1});
  if constexpr (std::is_floating_point_v<TypeParam>) {
    x.template values<TypeParam>()[2] = TypeParam(NAN);
    const auto expected = makeVariable<TypeParam>(Values{3});
    EXPECT_EQ(nansum(x), expected);
  } else {
    const auto expected = makeVariable<TypeParam>(Values{5});
    EXPECT_EQ(nansum(x), expected);
  }
}
TYPED_TEST(NansumTest, nansum_with_dim) {
  auto x = makeVariable<TypeParam>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                   Values{1.0, 2.0, 3.0, 4.0});
  if constexpr (std::is_floating_point_v<TypeParam>) {
    x.template values<TypeParam>()[2] = TypeParam(NAN);
    const auto expected =
        makeVariable<TypeParam>(Dims{Dim::Y}, Shape{2}, Values{1, 6});
    EXPECT_EQ(nansum(x, Dim::X), expected);
  } else {
    const auto expected =
        makeVariable<TypeParam>(Dims{Dim::Y}, Shape{2}, Values{4, 6});
    EXPECT_EQ(nansum(x, Dim::X), expected);
  }
}

class ReduceBinnedTest : public ::testing::Test {
protected:
  Variable indices =
      makeVariable<index_pair>(Dims{Dim::Y, Dim::Z}, Shape{2, 2},
                               Values{std::pair{0, 2}, std::pair{2, 2},
                                      std::pair{2, 5}, std::pair{5, 6}});
  Variable buffer = makeVariable<double>(Dims{Dim::X}, Shape{6}, sc_units::m,
                                         Values{1, 2, 3, 4, 5, 6},
                                         Variances{1, 2, 3, 4, 5, 6});
  Variable binned = make_bins(indices, Dim::X, buffer);
};

TEST_F(ReduceBinnedTest, all_dims) {
  EXPECT_EQ(sum(binned), sum(buffer));
  EXPECT_EQ(max(binned), max(buffer));
  EXPECT_EQ(min(binned), min(binned));
}

TEST_F(ReduceBinnedTest, all_dims_of_0d) {
  const auto binned0d = make_bins(
      makeVariable<index_pair>(Values{std::pair{0, 6}}), Dim::X, buffer);
  EXPECT_EQ(sum(binned0d), sum(buffer));
  EXPECT_EQ(max(binned0d), max(buffer));
  EXPECT_EQ(min(binned0d), min(binned));
}

TEST_F(ReduceBinnedTest, one_dim) {
  EXPECT_EQ(sum(binned, Dim::Y),
            makeVariable<double>(Dims{Dim::Z}, Shape{2}, sc_units::m,
                                 Values{15, 6}, Variances{15, 6}));
  EXPECT_EQ(sum(binned, Dim::Z),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m,
                                 Values{3, 18}, Variances{3, 18}));
  EXPECT_EQ(mean(binned), mean(buffer));
}

TEST_F(ReduceBinnedTest, slice) {
  EXPECT_EQ(sum(binned.slice({Dim::Y, 1, 2})),
            sum(buffer.slice({Dim::X, 2, 6})));
  EXPECT_EQ(mean(binned.slice({Dim::Y, 1, 2})),
            mean(buffer.slice({Dim::X, 2, 6})));
}
