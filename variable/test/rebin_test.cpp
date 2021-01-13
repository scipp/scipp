// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/bins.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/rebin.h"
#include "scipp/variable/variable.h"

using namespace scipp;

TEST(RebinTest, inner) {
  const auto base = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::counts,
                                         Values{1.0, 2.0});
  const auto oldEdge =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.0, 2.0, 3.0});
  const auto newEdge =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 3.0});
  for (const auto &var :
       {base, astype(base, dtype<int64_t>), astype(base, dtype<int32_t>)}) {
    EXPECT_EQ(rebin(var, Dim::X, oldEdge, newEdge),
              makeVariable<double>(Dims{Dim::X}, Shape{1}, units::counts,
                                   Values{3.0}));
  }
}

TEST(RebinTest, inner_descending) {
  auto var = makeVariable<double>(
      Dims{Dim::X}, Shape{10},
      Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0});
  var.setUnit(units::counts);
  const auto oldEdge = makeVariable<double>(
      Dims{Dim::X}, Shape{11},
      Values{10.0, 9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0, 0.0});
  const auto newEdge = makeVariable<double>(
      Dims{Dim::X}, Shape{6}, Values{11.0, 7.5, 6.0, 4.5, 2.0, 0.0});
  auto rebinned = rebin(var, Dim::X, oldEdge, newEdge);

  auto expected = makeVariable<double>(Dims{Dim::X}, Shape{5},
                                       Values{4.5, 5.5, 8.0, 18.0, 19.0});
  expected.setUnit(units::counts);

  ASSERT_EQ(rebinned, expected);
}

TEST(RebinTest, outer) {
  auto base =
      makeVariable<double>(Dimensions{{Dim::Y, 6}, {Dim::X, 2}}, units::counts,
                           Values{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6});
  const auto oldEdge =
      makeVariable<double>(Dims{Dim::Y}, Shape{7}, Values{1, 2, 3, 4, 5, 6, 7});
  const auto newEdge =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{0, 3, 8});

  for (const auto &var :
       {base, astype(base, dtype<int64_t>), astype(base, dtype<int32_t>)}) {
    EXPECT_EQ(rebin(var, Dim::Y, oldEdge, newEdge),

              makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                   units::counts, Values{4, 6, 14, 18}));
  }
}

TEST(RebinTest, outer_increasing) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{4, 1},
                                        Values{1, 2, 3, 4}, units::counts);
  constexpr auto varY = [](const auto... vals) {
    return makeVariable<double>(Dims{Dim::Y}, Shape{sizeof...(vals)},
                                Values{vals...});
  };
  const auto oldY = varY(0, 1, 2, 3, 4);
  constexpr auto var1x1 = [](const double value) {
    return makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 1},
                                Values{value}, units::counts);
  };
  // full range
  EXPECT_EQ(rebin(var, Dim::Y, oldY, oldY), var);
  // aligned old/bew edges
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(0, 4)), var1x1(10));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(0, 2)), var1x1(3));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(1, 3)), var1x1(5));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(2, 4)), var1x1(7));
  // crossing 0 bin bounds
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(0.1, 0.3)), var1x1((0.3 - 0.1) * 1));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(1.1, 1.3)), var1x1((1.3 - 1.1) * 2));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(3.1, 3.3)), var1x1((3.3 - 3.1) * 4));
  // crossing 1 bin bound
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(0.1, 2.0)), var1x1(0.9 * 1 + 2));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(0.1, 1.3)),
            var1x1((1.0 - 0.1) * 1 + (1.3 - 1.0) * 2));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(1.1, 2.3)),
            var1x1((2.0 - 1.1) * 2 + (2.3 - 2.0) * 3));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(2.1, 3.3)),
            var1x1((3.0 - 2.1) * 3 + (3.3 - 3.0) * 4));
  // crossing 2 bin bounds
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(0.1, 2.3)),
            var1x1((1.0 - 0.1) * 1 + 2 + (2.3 - 2.0) * 3));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(1.1, 3.3)),
            var1x1((2.0 - 1.1) * 2 + 3 + (3.3 - 3.0) * 4));
}

TEST(RebinTest, outer_decreasing) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{4, 1},
                                        Values{4, 3, 2, 1}, units::counts);
  constexpr auto varY = [](const auto... vals) {
    return makeVariable<double>(Dims{Dim::Y}, Shape{sizeof...(vals)},
                                Values{vals...});
  };
  const auto oldY = varY(4, 3, 2, 1, 0);
  constexpr auto var1x1 = [](const double value) {
    return makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 1},
                                Values{value}, units::counts);
  };
  // full range
  EXPECT_EQ(rebin(var, Dim::Y, oldY, oldY), var);
  // aligned old/bew edges
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(4, 0)), var1x1(10));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(2, 0)), var1x1(3));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(3, 1)), var1x1(5));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(4, 2)), var1x1(7));
  // crossing 0 bin bounds
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(0.3, 0.1)), var1x1((0.3 - 0.1) * 1));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(1.3, 1.1)), var1x1((1.3 - 1.1) * 2));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(3.3, 3.1)), var1x1((3.3 - 3.1) * 4));
  // crossing 1 bin bound
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(2.0, 0.1)), var1x1(0.9 * 1 + 2));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(1.3, 0.1)),
            var1x1((1.0 - 0.1) * 1 + (1.3 - 1.0) * 2));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(2.3, 1.1)),
            var1x1((2.0 - 1.1) * 2 + (2.3 - 2.0) * 3));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(3.3, 2.1)),
            var1x1((3.0 - 2.1) * 3 + (3.3 - 3.0) * 4));
  // crossing 2 bin bounds
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(2.3, 0.1)),
            var1x1((1.0 - 0.1) * 1 + 2 + (2.3 - 2.0) * 3));
  EXPECT_EQ(rebin(var, Dim::Y, oldY, varY(3.3, 1.1)),
            var1x1((2.0 - 1.1) * 2 + 3 + (3.3 - 3.0) * 4));
}

class RebinMask1DTest : public ::testing::Test {
protected:
  Variable x = makeVariable<double>(Dimensions{Dim::X, 11},
                                    Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});

  Variable mask = makeVariable<bool>(Dimensions{Dim::X, 10},
                                     Values{false, false, true, false, false,
                                            false, false, false, false, false});
};

TEST_F(RebinMask1DTest, mask_1d) {
  const auto edges =
      makeVariable<double>(Dimensions{Dim::X, 5}, Values{1, 3, 5, 7, 10});
  const auto expected = makeVariable<bool>(Dimensions{Dim::X, 4},
                                           Values{false, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}

TEST_F(RebinMask1DTest, mask_weights_1d) {
  const auto edges = makeVariable<double>(Dimensions{Dim::X, 5},
                                          Values{1.0, 3.5, 5.5, 7.0, 10.0});
  const auto expected = makeVariable<bool>(Dimensions{Dim::X, 4},
                                           Values{true, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}

TEST(Variable, rebin_mask_2d) {
  Variable x = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 6}},
                                    Values{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6});

  Variable mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 5}},
                                     Values{false, true, false, false, true,
                                            false, false, true, false, false});

  const auto edges = makeVariable<double>(Dimensions{Dim::X, 5},
                                          Values{1.0, 3.0, 4.0, 5.5, 6.0});
  const auto expected = makeVariable<bool>(
      Dimensions{{Dim::Y, 2}, {Dim::X, 4}},
      Values{true, false, true, true, false, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}

TEST(Variable, rebin_mask_outer) {
  const auto mask =
      makeVariable<bool>(Dimensions{{Dim::Y, 5}, {Dim::X, 2}},
                         Values{false, true, false, false, true, false, false,
                                true, false, false});

  const auto oldEdge =
      makeVariable<double>(Dimensions{Dim::Y, 6}, Values{1, 2, 3, 4, 5, 6});

  const auto newEdge =
      makeVariable<double>(Dimensions{Dim::Y, 4}, Values{0.0, 2.0, 3.5, 6.5});
  const auto expected =
      makeVariable<bool>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}},
                         Values{false, true, true, false, true, true});

  const auto result = rebin(mask, Dim::Y, oldEdge, newEdge);

  ASSERT_EQ(result, expected);
}

TEST(Variable, rebin_mask_outer_single) {
  const auto mask =
      makeVariable<bool>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}},
                         Values{false, true, false, false, false, false});

  const auto oldEdge =
      makeVariable<double>(Dimensions{Dim::Y, 4}, Values{1, 3, 5, 6});

  const auto newEdge =
      makeVariable<double>(Dimensions{Dim::Y, 2}, Values{0.0, 6.5});
  const auto expected = makeVariable<bool>(Dimensions{{Dim::Y, 1}, {Dim::X, 2}},
                                           Values{false, true});

  const auto result = rebin(mask, Dim::Y, oldEdge, newEdge);

  ASSERT_EQ(result, expected);
}
TEST(Variable, check_rebin_cannot_be_used_on_bin_data) {
  Dimensions dims{Dim::Y, 1};
  Variable buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 3}});
  Variable var = make_bins(indices, Dim::X, buffer);
  const auto oldEdge =
      makeVariable<double>(Dimensions{Dim::Y, 2}, Values{1, 4});
  const auto newEdge =
      makeVariable<double>(Dimensions{Dim::Y, 4}, Values{0, 1, 2, 3});
  EXPECT_THROW([[maybe_unused]] auto res = rebin(var, Dim::Y, oldEdge, newEdge),
               except::BucketError);
}
