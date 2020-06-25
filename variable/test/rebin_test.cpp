// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/rebin.h"
#include "scipp/variable/variable.h"

using namespace scipp;

TEST(Variable, rebin) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  var.setUnit(units::counts);
  const auto oldEdge =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.0, 2.0, 3.0});
  const auto newEdge =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 3.0});
  auto rebinned = rebin(var, Dim::X, oldEdge, newEdge);
  ASSERT_EQ(rebinned.dims().shape().size(), 1);
  ASSERT_EQ(rebinned.dims().volume(), 1);
  ASSERT_EQ(rebinned.values<double>().size(), 1);
  EXPECT_EQ(rebinned.values<double>()[0], 3.0);
}

TEST(Variable, rebin_outer) {
  auto var = makeVariable<double>(Dimensions{{Dim::Y, 6}, {Dim::X, 2}},
                                  Values{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6});
  var.setUnit(units::counts);
  const auto oldEdge =
      makeVariable<double>(Dims{Dim::Y}, Shape{7}, Values{1, 2, 3, 4, 5, 6, 7});
  const auto newEdge =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{0, 3, 8});
  auto rebinned = rebin(var, Dim::Y, oldEdge, newEdge);
  ASSERT_EQ(rebinned.dims().volume(), 4);
  ASSERT_EQ(rebinned.values<double>().size(), 4);

  auto expected = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                       Values{4, 6, 14, 18});
  expected.setUnit(units::counts);

  ASSERT_EQ(rebinned, expected);
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
