// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

class RebinTest : public ::testing::Test {
protected:
  Variable counts = makeVariable<double>(
      {{Dim::Y, 2}, {Dim::X, 4}}, units::counts, {1, 2, 3, 4, 5, 6, 7, 8});
  Variable x = makeVariable<double>({Dim::X, 5}, {1, 2, 3, 4, 5});
  Variable y = makeVariable<double>({Dim::Y, 3}, {1, 2, 3});
  DataArray array{counts, {{Dim::X, x}, {Dim::Y, y}}, {}};
  DataArray array_with_variances{
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 4}}, units::counts,
                           {1, 2, 3, 4, 5, 6, 7, 8},
                           {9, 10, 11, 12, 13, 14, 15, 16}),
      {{Dim::X, x}, {Dim::Y, y}},
      {}};
};

TEST_F(RebinTest, inner_data_array) {
  auto edges = makeVariable<double>({Dim::X, 3}, {1, 3, 5});
  DataArray expected(makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                          units::counts, {3, 7, 11, 15}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {});

  ASSERT_EQ(rebin(array, Dim::X, edges), expected);
}

TEST_F(RebinTest, inner_data_array_with_variances) {
  auto edges = makeVariable<double>({Dim::X, 3}, {1, 3, 5});
  DataArray expected(makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                          units::counts, {3, 7, 11, 15},
                                          {19, 23, 27, 31}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {});

  ASSERT_EQ(rebin(array_with_variances, Dim::X, edges), expected);
}

TEST_F(RebinTest, inner_data_array_unaligned_edges) {
  auto edges = makeVariable<double>({Dim::X, 3}, {1.5, 3.5, 5.5});
  DataArray expected(makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                          units::counts,
                                          {0.5 * 1 + 2 + 0.5 * 3, 0.5 * 3 + 4,
                                           0.5 * 5 + 6 + 0.5 * 7, 0.5 * 7 + 8}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {});

  ASSERT_EQ(rebin(array, Dim::X, edges), expected);
}

TEST_F(RebinTest, outer_data_array) {
  auto edges = makeVariable<double>({Dim::Y, 2}, {1, 3});
  DataArray expected(makeVariable<double>({{Dim::Y, 1}, {Dim::X, 4}},
                                          units::counts, {6, 8, 10, 12}),
                     {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array, Dim::Y, edges), expected);
}

TEST_F(RebinTest, outer_data_array_with_variances) {
  auto edges = makeVariable<double>({Dim::Y, 2}, {1, 3});
  DataArray expected(makeVariable<double>({{Dim::Y, 1}, {Dim::X, 4}},
                                          units::counts, {6, 8, 10, 12},
                                          {22, 24, 26, 28}),
                     {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array_with_variances, Dim::Y, edges), expected);
}

TEST_F(RebinTest, outer_data_array_unaligned_edges) {
  auto edges = makeVariable<double>({Dim::Y, 3}, {1.0, 2.5, 3.5});
  DataArray expected(
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 4}}, units::counts,
                           {1.0 + 0.5 * 5, 2.0 + 0.5 * 6, 3.0 + 0.5 * 7,
                            4.0 + 0.5 * 8, 0.5 * 5, 0.5 * 6, 0.5 * 7, 0.5 * 8}),
      {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array, Dim::Y, edges), expected);
}

TEST_F(RebinTest, keeps_unrelated_labels_but_drops_others) {
  const auto labels_x = makeVariable<double>({Dim::X, 4});
  const auto labels_y = makeVariable<double>({Dim::Y, 2});
  const DataArray a(counts, {{Dim::X, x}, {Dim::Y, y}},
                    {{"x", labels_x}, {"y", labels_y}});
  const auto edges = makeVariable<double>({Dim::X, 3}, {1, 3, 5});
  DataArray expected(makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                          units::counts, {3, 7, 11, 15}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {{"y", labels_y}});

  ASSERT_EQ(rebin(a, Dim::X, edges), expected);
}

class RebinMask1DTest : public ::testing::Test {
protected:
  Variable x =
      makeVariable<double>({Dim::X, 11}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});

  Variable mask =
      makeVariable<bool>({Dim::X, 10}, {false, false, true, false, false, false,
                                        false, false, false, false});
};

TEST_F(RebinMask1DTest, mask_1d) {

  const auto edges = makeVariable<double>({Dim::X, 5}, {1, 3, 5, 7, 10});
  const auto expected =
      makeVariable<bool>({Dim::X, 4}, {false, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}

TEST_F(RebinMask1DTest, mask_weights_1d) {
  const auto edges =
      makeVariable<double>({Dim::X, 5}, {1.0, 3.5, 5.5, 7.0, 10.0});
  const auto expected =
      makeVariable<bool>({Dim::X, 4}, {true, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}

class RebinMask2DTest : public ::testing::Test {
protected:
  Variable x = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 6}},
                                    {1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6});

  Variable mask = makeVariable<bool>(
      {{Dim::Y, 2}, {Dim::X, 5}},
      {false, true, false, false, true, false, false, true, false, false});
};

TEST_F(RebinMask2DTest, mask_weights_2d) {
  const auto edges =
      makeVariable<double>({Dim::X, 5}, {1.0, 3.0, 4.0, 5.5, 6.0});
  const auto expected =
      makeVariable<bool>({{Dim::Y, 2}, {Dim::X, 4}},
                         {true, false, true, true, false, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}
