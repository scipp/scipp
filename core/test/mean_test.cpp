// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(MeanTest, unknown_dim_fail) {
  const auto var =
      createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                             units::Unit(units::m), Values{1.0, 2.0, 3.0, 4.0});
  EXPECT_THROW(mean(var, Dim::Z), except::DimensionError);
}

TEST(MeanTest, sparse_dim_fail) {
  const auto var =
      createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, Dimensions::Sparse},
                             units::Unit(units::m));
  EXPECT_THROW(mean(var, Dim::X), except::DimensionError);
  EXPECT_THROW(mean(var, Dim::Y), except::DimensionError);
  EXPECT_THROW(mean(var, Dim::Z), except::DimensionError);
}

TEST(MeanTest, basic) {
  const auto var =
      createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                             units::Unit(units::m), Values{1.0, 2.0, 3.0, 4.0});
  const auto meanX = createVariable<double>(
      Dims{Dim::Y}, Shape{2}, units::Unit(units::m), Values{1.5, 3.5});
  const auto meanY = createVariable<double>(
      Dims{Dim::X}, Shape{2}, units::Unit(units::m), Values{2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, masked_data_array) {
  const auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, units::m,
                                        {1.0, 2.0, 3.0, 4.0});
  const auto mask = makeVariable<bool>({Dim::X, 2}, {false, true});
  DataArray a(var);
  a.masks().set("mask", mask);
  const auto meanX = makeVariable<double>({Dim::Y, 2}, units::m, {1.0, 3.0});
  const auto meanY = makeVariable<double>({Dim::X, 2}, units::m, {2.0, 3.0});
  EXPECT_EQ(mean(a, Dim::X).data(), meanX);
  EXPECT_EQ(mean(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(mean(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(mean(a, Dim::Y).masks().contains("mask"));
}

TEST(MeanTest, masked_data_array_two_masks) {
  const auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, units::m,
                                        {1.0, 2.0, 3.0, 4.0});
  const auto maskX = makeVariable<bool>({Dim::X, 2}, {false, true});
  const auto maskY = makeVariable<bool>({Dim::Y, 2}, {false, true});
  DataArray a(var);
  a.masks().set("x", maskX);
  a.masks().set("y", maskY);
  const auto meanX = makeVariable<double>({Dim::Y, 2}, units::m, {1.0, 3.0});
  const auto meanY = makeVariable<double>({Dim::X, 2}, units::m, {1.0, 2.0});
  EXPECT_EQ(mean(a, Dim::X).data(), meanX);
  EXPECT_EQ(mean(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(mean(a, Dim::X).masks().contains("x"));
  EXPECT_TRUE(mean(a, Dim::X).masks().contains("y"));
  EXPECT_TRUE(mean(a, Dim::Y).masks().contains("x"));
  EXPECT_FALSE(mean(a, Dim::Y).masks().contains("y"));
}

TEST(MeanTest, dtype_float_preserved) {
  const auto var =
      createVariable<float>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                            units::Unit(units::m), Values{1.0, 2.0, 3.0, 4.0});
  const auto meanX = createVariable<float>(
      Dims{Dim::Y}, Shape{2}, units::Unit(units::m), Values{1.5, 3.5});
  const auto meanY = createVariable<float>(
      Dims{Dim::X}, Shape{2}, units::Unit(units::m), Values{2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, dtype_int_gives_double_mean) {
  const auto var =
      createVariable<int32_t>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                              units::Unit(units::m), Values{1, 2, 3, 4});
  const auto meanX = createVariable<double>(
      Dims{Dim::Y}, Shape{2}, units::Unit(units::m), Values{1.5, 3.5});
  const auto meanY = createVariable<double>(
      Dims{Dim::X}, Shape{2}, units::Unit(units::m), Values{2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, variances_as_standard_deviation_of_the_mean) {
  const auto var = createVariable<double>(
      Dims{Dim::Y, Dim::X}, Shape{2, 2}, units::Unit(units::m),
      Values{1.0, 2.0, 3.0, 4.0}, Variances{5.0, 6.0, 7.0, 8.0});
  const auto meanX =
      createVariable<double>(Dims{Dim::Y}, Shape{2}, units::Unit(units::m),
                             Values{1.5, 3.5}, Variances{0.5 * 5.5, 0.5 * 7.5});
  const auto meanY =
      createVariable<double>(Dims{Dim::X}, Shape{2}, units::Unit(units::m),
                             Values{2.0, 3.0}, Variances{0.5 * 6.0, 0.5 * 7.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, dataset_mean_fails) {
  Dataset d;
  d.setData("a", createVariable<double>(Dims{Dim::X}, Shape{2}));
  d.setData("b", createVariable<double>(Values{1.0}));
  // "b" does not depend on X, so this fails. This could change in the future if
  // we find a clear definition of the functions behavior in this case.
  EXPECT_THROW(mean(d, Dim::X), except::DimensionError);
}
