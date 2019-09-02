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
  const auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, units::m,
                                        {1.0, 2.0, 3.0, 4.0});
  EXPECT_THROW(mean(var, Dim::Z), except::DimensionError);
}

TEST(MeanTest, sparse_dim_fail) {
  const auto var = makeVariable<double>(
      {{Dim::Y, 2}, {Dim::X, Dimensions::Sparse}}, units::m);
  EXPECT_THROW(mean(var, Dim::X), except::DimensionError);
  EXPECT_THROW(mean(var, Dim::Y), except::DimensionError);
  EXPECT_THROW(mean(var, Dim::Z), except::DimensionError);
}

TEST(MeanTest, basic) {
  const auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, units::m,
                                        {1.0, 2.0, 3.0, 4.0});
  const auto meanX = makeVariable<double>({Dim::Y, 2}, units::m, {1.5, 3.5});
  const auto meanY = makeVariable<double>({Dim::X, 2}, units::m, {2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, dtype_float_preserved) {
  const auto var = makeVariable<float>({{Dim::Y, 2}, {Dim::X, 2}}, units::m,
                                       {1.0, 2.0, 3.0, 4.0});
  const auto meanX = makeVariable<float>({Dim::Y, 2}, units::m, {1.5, 3.5});
  const auto meanY = makeVariable<float>({Dim::X, 2}, units::m, {2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, dtype_int_gives_double_mean) {
  const auto var =
      makeVariable<int32_t>({{Dim::Y, 2}, {Dim::X, 2}}, units::m, {1, 2, 3, 4});
  const auto meanX = makeVariable<double>({Dim::Y, 2}, units::m, {1.5, 3.5});
  const auto meanY = makeVariable<double>({Dim::X, 2}, units::m, {2.0, 3.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, variances_as_standard_deviation_of_the_mean) {
  const auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, units::m,
                           {1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0});
  const auto meanX = makeVariable<double>({Dim::Y, 2}, units::m, {1.5, 3.5},
                                          {0.5 * 5.5, 0.5 * 7.5});
  const auto meanY = makeVariable<double>({Dim::X, 2}, units::m, {2.0, 3.0},
                                          {0.5 * 6.0, 0.5 * 7.0});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(mean(var, Dim::Y), meanY);
}

TEST(MeanTest, dataset_mean_fails) {
  Dataset d;
  d.setData("a", makeVariable<double>({Dim::X, 2}));
  d.setData("b", makeVariable<double>(1.0));
  // "b" does not depend on X, so this fails. This could change in the future if
  // we find a clear definition of the functions behavior in this case.
  EXPECT_THROW(mean(d, Dim::X), except::DimensionError);
}
