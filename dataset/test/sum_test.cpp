// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "scipp/dataset/reduction.h"

#include <gtest/gtest.h>
#include <limits>
#include <vector>

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
  EXPECT_TRUE(sum(a, Dim::Y).masks().contains("mask"));
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

TEST_F(Sum2dCoordTest, data_array_2d_labels) {
  DataArray a(var, {{Dim("xlabels"), var}});
  // Labels are for summed dimension -> drop. Note that associated dimension for
  // labels is their inner dim, X in this case.
  EXPECT_FALSE(sum(a, Dim::X).coords().contains(Dim("xlabels")));
}

TEST_F(Sum2dCoordTest, data_array_bad_2d_coord_fail) {
  DataArray a(var, {{Dim::X, var}});
  // Values being summed have different X coord -> fail.
  EXPECT_THROW(sum(a, Dim::Y), except::DimensionError);
}

TEST_F(Sum2dCoordTest, data_array_bad_2d_labels_fail) {
  DataArray a(var, {{Dim("xlabels"), var}});
  // Values being summed have different x labels -> fail.
  EXPECT_THROW(sum(a, Dim::Y), except::DimensionError);
}
