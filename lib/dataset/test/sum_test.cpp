// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <limits>
#include <vector>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/sum.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(SumTest, masked_data_array) {
  const auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);
  const auto sumX =
      makeVariable<double>(Dimensions{Dim::Y, 2}, units::m, Values{1.0, 3.0});
  const auto sumY =
      makeVariable<double>(Dimensions{Dim::X, 2}, units::m, Values{4.0, 6.0});
  EXPECT_EQ(sum(a, Dim::X).data(), sumX);
  EXPECT_EQ(sum(a, Dim::Y).data(), sumY);
  EXPECT_FALSE(sum(a, Dim::X).masks().contains("mask"));
  auto summedY = sum(a, Dim::Y);
  EXPECT_TRUE(summedY.masks().contains("mask"));
  // Ensure reduction operation does NOT share the unrelated mask
  EXPECT_EQ(summedY.masks()["mask"], mask);
  summedY.masks()["mask"] &= ~mask;
  EXPECT_NE(summedY.masks()["mask"], mask);
}

TEST(SumTest, masked_data_with_special_vals) {
  const auto var = makeVariable<double>(
      Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, units::m,
      Values{1.0, std::numeric_limits<double>::quiet_NaN(), 3.0, 4.0});
  const auto mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                       Values{false, true, false, false});

  DataArray a(var);
  a.masks().set("mask", mask);

  const auto sumX =
      makeVariable<double>(Dimensions{Dim::Y, 2}, units::m, Values{1.0, 7.0});
  const auto sumY =
      makeVariable<double>(Dimensions{Dim::X, 2}, units::m, Values{4.0, 4.0});

  EXPECT_EQ(sum(a, Dim::X).data(), sumX);
  EXPECT_EQ(sum(a, Dim::Y).data(), sumY);
}

TEST(SumTest, masked_data_array_two_masks) {
  const auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto maskX =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  const auto maskY =
      makeVariable<bool>(Dimensions{Dim::Y, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("x", maskX);
  a.masks().set("y", maskY);
  const auto sumX =
      makeVariable<double>(Dimensions{Dim::Y, 2}, units::m, Values{1.0, 3.0});
  const auto sumY =
      makeVariable<double>(Dimensions{Dim::X, 2}, units::m, Values{1.0, 2.0});
  EXPECT_EQ(sum(a, Dim::X).data(), sumX);
  EXPECT_EQ(sum(a, Dim::Y).data(), sumY);
  EXPECT_FALSE(sum(a, Dim::X).masks().contains("x"));
  EXPECT_TRUE(sum(a, Dim::X).masks().contains("y"));
  EXPECT_TRUE(sum(a, Dim::Y).masks().contains("x"));
  EXPECT_FALSE(sum(a, Dim::Y).masks().contains("y"));
}

class Sum2dCoordTest : public ::testing::Test {
protected:
  Variable var{makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    Values{1.0, 2.0, 3.0, 4.0})};
};

TEST_F(Sum2dCoordTest, data_array_2d_coord) {
  DataArray a(var, {{Dim::X, var}});
  // Coord is for summed dimension -> drop.
  EXPECT_FALSE(sum(a, Dim::X).coords().contains(Dim::X));
}

TEST_F(Sum2dCoordTest, data_array_2d_labels_dropped) {
  DataArray a(var, {{Dim("xlabels"), var}});
  // Coord depend on summed dimension -> drop.
  EXPECT_FALSE(sum(a, Dim::X).coords().contains(Dim("xlabels")));
  EXPECT_FALSE(sum(a, Dim::Y).coords().contains(Dim("xlabels")));
  // Drops even dimension coords
  EXPECT_FALSE(sum(a, Dim::Y).coords().contains(Dim::X));
}

TEST_F(Sum2dCoordTest, data_array_independent_2d_labels_preserved) {
  Variable var3d{makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X},
                                      Shape{1, 2, 2},
                                      Values{1.0, 2.0, 3.0, 4.0})};
  DataArray a(var3d, {{Dim::X, var}});
  EXPECT_FALSE(sum(a, Dim::X).coords().contains(Dim::X));
  EXPECT_FALSE(sum(a, Dim::Y).coords().contains(Dim::X));
  EXPECT_TRUE(sum(a, Dim::Z).coords().contains(Dim::X));
}

class ReduceBinnedTest : public ::testing::Test {
protected:
  Variable indices = makeVariable<index_pair>(
      Dims{Dim::Y}, Shape{3},
      Values{std::pair{0, 2}, std::pair{2, 2}, std::pair{2, 5}});
  Variable data = makeVariable<double>(Dims{Dim::X}, Shape{5}, units::m,
                                       Values{1, 2, 3, 4, 5});
  DataArray buffer = DataArray(data);
  Variable binned = make_bins(indices, Dim::X, buffer);
};

TEST_F(ReduceBinnedTest, sum_masked) {
  buffer.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::X}, Shape{5},
                                 Values{true, true, false, true, false}));
  binned = make_bins(indices, Dim::X, buffer);
  EXPECT_EQ(sum(binned), makeVariable<double>(Dims{}, units::m, Values{3 + 5}));
}

TEST_F(ReduceBinnedTest, mean_masked) {
  buffer.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::X}, Shape{5},
                                 Values{true, true, false, true, false}));
  binned = make_bins(indices, Dim::X, buffer);
  EXPECT_EQ(mean(binned),
            makeVariable<double>(Dims{}, units::m, Values{(3.0 + 5.0) / 2}));
}
