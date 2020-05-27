// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/core/except.h"
#include "scipp/dataset/reduction.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(MeanTest, unknown_dim_fail) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  EXPECT_THROW(const auto view = mean(var, Dim::Z), except::DimensionError);
}

TEST(MeanTest, event_fail) {
  const auto var =
      makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2}, units::m);
  EXPECT_THROW(const auto view = mean(var, Dim::X), except::DimensionError);
  EXPECT_THROW(const auto view = mean(var, Dim::Y), except::TypeError);
  EXPECT_THROW(const auto view = mean(var, Dim::Z), except::DimensionError);
}

TEST(MeanTest, basic) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto meanX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5});
  const auto meanY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, basic_in_place) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  auto meanX = makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m);
  auto meanY = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m);
  auto viewX = mean(var, Dim::X, meanX);
  auto viewY = mean(var, Dim::Y, meanY);
  const auto expectedX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5});
  const auto expectedY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0});
  EXPECT_EQ(meanX, expectedX);
  EXPECT_EQ(viewX, meanX);
  EXPECT_EQ(viewX.underlying(), meanX);
  EXPECT_EQ(meanY, expectedY);
  EXPECT_EQ(viewY, meanY);
  EXPECT_EQ(viewY.underlying(), meanY);
}

TEST(MeanTest, in_place_fail_output_dtype) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  auto out = makeVariable<int>(Dims{Dim::Y}, Shape{2}, units::m);
  EXPECT_THROW(const auto view = mean(var, Dim::X, out), except::UnitError);
}

TEST(MeanTest, masked_data_array) {
  const auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);
  const auto meanX =
      makeVariable<double>(Dimensions{Dim::Y, 2}, units::m, Values{1.0, 3.0});
  const auto meanY =
      makeVariable<double>(Dimensions{Dim::X, 2}, units::m, Values{2.0, 3.0});
  EXPECT_EQ(mean(a, Dim::X).data(), meanX);
  EXPECT_EQ(mean(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(mean(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(mean(a, Dim::Y).masks().contains("mask"));
}

TEST(MeanTest, masked_data_array_two_masks) {
  const auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto maskX =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  const auto maskY =
      makeVariable<bool>(Dimensions{Dim::Y, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("x", maskX);
  a.masks().set("y", maskY);
  const auto meanX =
      makeVariable<double>(Dimensions{Dim::Y, 2}, units::m, Values{1.0, 3.0});
  const auto meanY =
      makeVariable<double>(Dimensions{Dim::X, 2}, units::m, Values{1.0, 2.0});
  EXPECT_EQ(mean(a, Dim::X).data(), meanX);
  EXPECT_EQ(mean(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(mean(a, Dim::X).masks().contains("x"));
  EXPECT_TRUE(mean(a, Dim::X).masks().contains("y"));
  EXPECT_TRUE(mean(a, Dim::Y).masks().contains("x"));
  EXPECT_FALSE(mean(a, Dim::Y).masks().contains("y"));
}

TEST(MeanTest, dtype_float_preserved) {
  const auto var = makeVariable<float>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                       units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto meanX =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5});
  const auto meanY =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, dtype_int_gives_double_mean) {
  const auto var = makeVariable<int32_t>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                         units::m, Values{1, 2, 3, 4});
  const auto meanX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5});
  const auto meanY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, variances_as_standard_deviation_of_the_mean) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0},
                                        Variances{5.0, 6.0, 7.0, 8.0});
  const auto meanX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5},
                           Variances{0.5 * 5.5, 0.5 * 7.5});
  const auto meanY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0},
                           Variances{0.5 * 6.0, 0.5 * 7.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, dataset_mean_fails) {
  Dataset d;
  d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{2}));
  d.setData("b", makeVariable<double>(Values{1.0}));
  // "b" does not depend on X, so this fails. This could change in the future if
  // we find a clear definition of the functions behavior in this case.
  EXPECT_THROW(mean(d, Dim::X), except::DimensionError);
}
