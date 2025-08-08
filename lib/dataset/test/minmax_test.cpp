// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <limits>
#include <vector>

#include "scipp/dataset/max.h"
#include "scipp/dataset/min.h"
#include "scipp/dataset/nanmax.h"
#include "scipp/dataset/nanmin.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(MaxTest, masked_data_array) {
  const auto var =
      makeVariable<double>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
                           Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  const auto max_x = makeVariable<double>(Dimensions{Dim::Y, 3}, sc_units::m,
                                          Values{1.0, 3.0, 5.0});
  const auto max_y = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                          Values{5.0, 6.0});
  EXPECT_EQ(max(a, Dim::X).data(), max_x);
  EXPECT_EQ(max(a, Dim::Y).data(), max_y);
  EXPECT_FALSE(max(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(max(a, Dim::Y).masks().contains("mask"));
}

TEST(MaxTest, masked_data_with_nan) {
  const auto var = makeVariable<double>(
      Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
      Values{1.0, 2.0, std::numeric_limits<double>::quiet_NaN(), 4.0});
  const auto mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                       Values{false, false, true, false});
  DataArray a(var);
  a.masks().set("mask", mask);

  const auto max_x = makeVariable<double>(Dimensions{Dim::Y, 2}, sc_units::m,
                                          Values{2.0, 4.0});
  const auto max_y = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                          Values{1.0, 4.0});
  EXPECT_EQ(max(a, Dim::X).data(), max_x);
  EXPECT_EQ(max(a, Dim::Y).data(), max_y);
  EXPECT_FALSE(max(a, Dim::X).masks().contains("mask"));
  EXPECT_FALSE(max(a, Dim::Y).masks().contains("mask"));
}

TEST(NanMaxTest, masked_data_array) {
  const auto var = makeVariable<double>(
      Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
      Values{1.0, std::numeric_limits<double>::quiet_NaN(), 3.0, 4.0,
             std::numeric_limits<double>::quiet_NaN(), 6.0});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  const auto max_x = makeVariable<double>(
      Dimensions{Dim::Y, 3}, sc_units::m,
      Values{1.0, 3.0, std::numeric_limits<double>::lowest()});
  const auto max_y = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                          Values{3.0, 6.0});
  EXPECT_EQ(nanmax(a, Dim::X).data(), max_x);
  EXPECT_EQ(nanmax(a, Dim::Y).data(), max_y);
  EXPECT_FALSE(nanmax(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(nanmax(a, Dim::Y).masks().contains("mask"));
}

TEST(MinTest, masked_data_array) {
  const auto var =
      makeVariable<double>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
                           Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  const auto min_x = makeVariable<double>(Dimensions{Dim::Y, 3}, sc_units::m,
                                          Values{1.0, 3.0, 5.0});
  const auto min_y = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                          Values{1.0, 2.0});
  EXPECT_EQ(min(a, Dim::X).data(), min_x);
  EXPECT_EQ(min(a, Dim::Y).data(), min_y);
  EXPECT_FALSE(min(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(min(a, Dim::Y).masks().contains("mask"));
}

TEST(MinTest, masked_data_with_nan) {
  const auto var = makeVariable<double>(
      Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
      Values{1.0, 2.0, std::numeric_limits<double>::quiet_NaN(), 4.0});
  const auto mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                       Values{false, false, true, false});
  DataArray a(var);
  a.masks().set("mask", mask);

  const auto min_x = makeVariable<double>(Dimensions{Dim::Y, 2}, sc_units::m,
                                          Values{1.0, 4.0});
  const auto min_y = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                          Values{1.0, 2.0});
  EXPECT_EQ(min(a, Dim::X).data(), min_x);
  EXPECT_EQ(min(a, Dim::Y).data(), min_y);
  EXPECT_FALSE(min(a, Dim::X).masks().contains("mask"));
  EXPECT_FALSE(min(a, Dim::Y).masks().contains("mask"));
}

TEST(NanMinTest, masked_data_array) {
  const auto var = makeVariable<double>(
      Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
      Values{1.0, std::numeric_limits<double>::quiet_NaN(), 3.0, 4.0,
             std::numeric_limits<double>::quiet_NaN(), 6.0});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  const auto min_x = makeVariable<double>(
      Dimensions{Dim::Y, 3}, sc_units::m,
      Values{1.0, 3.0, std::numeric_limits<double>::max()});
  const auto min_y = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                          Values{1.0, 4.0});
  EXPECT_EQ(nanmin(a, Dim::X).data(), min_x);
  EXPECT_EQ(nanmin(a, Dim::Y).data(), min_y);
  EXPECT_FALSE(nanmin(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(nanmin(a, Dim::Y).masks().contains("mask"));
}
