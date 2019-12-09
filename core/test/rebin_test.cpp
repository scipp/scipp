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
  Variable counts = createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 4},
                                           units::Unit(units::counts),
                                           Values{1, 2, 3, 4, 5, 6, 7, 8});
  Variable x =
      createVariable<double>(Dims{Dim::X}, Shape{5}, Values{1, 2, 3, 4, 5});
  Variable y = createVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3});
  DataArray array{counts, {{Dim::X, x}, {Dim::Y, y}}, {}};
  DataArray array_with_variances{
      createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 4},
                             units::Unit(units::counts),
                             Values{1, 2, 3, 4, 5, 6, 7, 8},
                             Variances{9, 10, 11, 12, 13, 14, 15, 16}),
      {{Dim::X, x}, {Dim::Y, y}},
      {}};
};

TEST_F(RebinTest, inner_data_array) {
  auto edges = createVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 3, 5});
  DataArray expected(createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                            units::Unit(units::counts),
                                            Values{3, 7, 11, 15}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {});

  ASSERT_EQ(rebin(array, Dim::X, edges), expected);
}

TEST_F(RebinTest, inner_data_array_with_variances) {
  auto edges = createVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 3, 5});
  DataArray expected(createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                            units::Unit(units::counts),
                                            Values{3, 7, 11, 15},
                                            Variances{19, 23, 27, 31}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {});

  ASSERT_EQ(rebin(array_with_variances, Dim::X, edges), expected);
}

TEST_F(RebinTest, inner_data_array_unaligned_edges) {
  auto edges =
      createVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.5, 3.5, 5.5});
  DataArray expected(
      createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                             units::Unit(units::counts),
                             Values{0.5 * 1 + 2 + 0.5 * 3, 0.5 * 3 + 4,
                                    0.5 * 5 + 6 + 0.5 * 7, 0.5 * 7 + 8}),
      {{Dim::X, edges}, {Dim::Y, y}}, {});

  ASSERT_EQ(rebin(array, Dim::X, edges), expected);
}

TEST_F(RebinTest, outer_data_array) {
  auto edges = createVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 3});
  DataArray expected(createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 4},
                                            units::Unit(units::counts),
                                            Values{6, 8, 10, 12}),
                     {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array, Dim::Y, edges), expected);
}

TEST_F(RebinTest, outer_data_array_with_variances) {
  auto edges = createVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 3});
  DataArray expected(createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 4},
                                            units::Unit(units::counts),
                                            Values{6, 8, 10, 12},
                                            Variances{22, 24, 26, 28}),
                     {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array_with_variances, Dim::Y, edges), expected);
}

TEST_F(RebinTest, outer_data_array_unaligned_edges) {
  auto edges =
      createVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1.0, 2.5, 3.5});
  DataArray expected(
      createVariable<double>(
          Dims{Dim::Y, Dim::X}, Shape{2, 4}, units::Unit(units::counts),
          Values{1.0 + 0.5 * 5, 2.0 + 0.5 * 6, 3.0 + 0.5 * 7, 4.0 + 0.5 * 8,
                 0.5 * 5, 0.5 * 6, 0.5 * 7, 0.5 * 8}),
      {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array, Dim::Y, edges), expected);
}

TEST_F(RebinTest, keeps_unrelated_labels_but_drops_others) {
  const auto labels_x = createVariable<double>(Dims{Dim::X}, Shape{4});
  const auto labels_y = createVariable<double>(Dims{Dim::Y}, Shape{2});
  const DataArray a(counts, {{Dim::X, x}, {Dim::Y, y}},
                    {{"x", labels_x}, {"y", labels_y}});
  const auto edges =
      createVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 3, 5});
  DataArray expected(createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                            units::Unit(units::counts),
                                            Values{3, 7, 11, 15}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {{"y", labels_y}});

  ASSERT_EQ(rebin(a, Dim::X, edges), expected);
}

class RebinMask1DTest : public ::testing::Test {
protected:
  Variable x = createVariable<double>(
      Dimensions{Dim::X, 11}, Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});

  Variable mask = createVariable<bool>(
      Dimensions{Dim::X, 10}, Values{false, false, true, false, false, false,
                                     false, false, false, false});
};

TEST_F(RebinMask1DTest, mask_1d) {

  const auto edges =
      createVariable<double>(Dimensions{Dim::X, 5}, Values{1, 3, 5, 7, 10});
  const auto expected = createVariable<bool>(Dimensions{Dim::X, 4},
                                             Values{false, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}

TEST_F(RebinMask1DTest, mask_weights_1d) {
  const auto edges = createVariable<double>(Dimensions{Dim::X, 5},
                                            Values{1.0, 3.5, 5.5, 7.0, 10.0});
  const auto expected = createVariable<bool>(Dimensions{Dim::X, 4},
                                             Values{true, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}

class RebinMask2DTest : public ::testing::Test {
protected:
  Variable x =
      createVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 6}},
                             Values{1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6});

  Variable mask =
      createVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 5}},
                           Values{false, true, false, false, true, false, false,
                                  true, false, false});
};

TEST_F(RebinMask2DTest, mask_weights_2d) {
  const auto edges = createVariable<double>(Dimensions{Dim::X, 5},
                                            Values{1.0, 3.0, 4.0, 5.5, 6.0});
  const auto expected = createVariable<bool>(
      Dimensions{{Dim::Y, 2}, {Dim::X, 4}},
      Values{true, false, true, true, false, true, false, false});

  const auto result = rebin(mask, Dim::X, x, edges);

  ASSERT_EQ(result, expected);
}
