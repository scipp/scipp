// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "scipp/core/except.h"
#include "scipp/variable/reduction.h"
#include "test_macros.h"
#include <gtest/gtest.h>
#include <scipp/common/overloaded.h>

namespace {
using namespace scipp;
using namespace scipp::dataset;

template <typename Op> void unknown_dim_fail(Op op) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  EXPECT_THROW(const auto view = op(var, Dim::Z), except::DimensionError);
}

template <typename Op> void basic(Op op) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto meanX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5});
  const auto meanY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0});
  EXPECT_EQ(op(var, Dim::X), meanX);
  EXPECT_EQ(op(var, Dim::Y), meanY);
}

template <typename Op> void basic_all_dims(Op op) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto meanAll = makeVariable<double>(units::m, Values{2.5});
  EXPECT_EQ(op(var), meanAll);
}

template <typename Op> void basic_in_place(Op op) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  auto meanX = makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m);
  auto meanY = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m);
  auto viewX = op(var, Dim::X, meanX);
  auto viewY = op(var, Dim::Y, meanY);
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

template <typename Op> void in_place_fail_output_dtype(Op op) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  auto out = makeVariable<int>(Dims{Dim::Y}, Shape{2}, units::m);
  EXPECT_THROW([[maybe_unused]] const auto view = op(var, Dim::X, out),
               except::UnitError);
}

template <typename Op> void dtype_float_preserved(Op op) {
  const auto var = makeVariable<float>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                       units::m, Values{1.0, 2.0, 3.0, 4.0});
  const auto meanX =
      makeVariable<float>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5});
  const auto meanY =
      makeVariable<float>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0});
  EXPECT_EQ(op(var, Dim::X), meanX);
  EXPECT_EQ(op(var, Dim::Y), meanY);
}

template <typename Op> void dtype_int_gives_double_mean(Op op) {
  const auto var = makeVariable<int32_t>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                         units::m, Values{1, 2, 3, 4});
  const auto meanX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5});
  const auto meanY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0});
  EXPECT_EQ(op(var, Dim::X), meanX);
  EXPECT_EQ(op(var, Dim::Y), meanY);
}

template <typename Op> void variances_as_standard_deviation_of_the_mean(Op op) {
  const auto var = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0},
                                        Variances{5.0, 6.0, 7.0, 8.0});
  const auto meanX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.5},
                           Variances{0.5 * 5.5, 0.5 * 7.5});
  const auto meanY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 3.0},
                           Variances{0.5 * 6.0, 0.5 * 7.0});
  EXPECT_EQ(op(var, Dim::X), meanX);
  EXPECT_EQ(op(var, Dim::Y), meanY);
}

auto mean_func =
    overloaded{[](const auto &var, const auto &dim) { return mean(var, dim); },
               [](const auto &var, const auto &dim, auto &out) {
                 return mean(var, dim, out);
               },
               [](const auto &var) { return mean(var); }};
auto nanmean_func = overloaded{
    [](const auto &var, const auto &dim) { return nanmean(var, dim); },
    [](const auto &var, const auto &dim, auto &out) {
      return nanmean(var, dim, out);
    },
    [](const auto &var) { return nanmean(var); }};
} // namespace

TEST(MeanTest, unknown_dim_fail) {
  unknown_dim_fail(mean_func);
  unknown_dim_fail(nanmean_func);
}

TEST(MeanTest, basic) {
  basic(mean_func);
  basic(nanmean_func);
}

TEST(MeanTest, basic_all_dims) {
  basic_all_dims(mean_func);
  basic_all_dims(nanmean_func);
}

TEST(MeanTest, basic_in_place) {
  basic_in_place(mean_func);
  basic_in_place(nanmean_func);
}

TEST(MeanTest, in_place_fail_output_dtype) {
  in_place_fail_output_dtype(mean_func);
  in_place_fail_output_dtype(nanmean_func);
}

TEST(MeanTest, dtype_float_preserved) {
  dtype_float_preserved(mean_func);
  dtype_float_preserved(nanmean_func);
}

TEST(MeanTest, dtype_int_gives_double_mean) {
  dtype_int_gives_double_mean(mean_func);
  dtype_int_gives_double_mean(nanmean_func);
}

TEST(MeanTest, variances_as_standard_deviation_of_the_mean) {
  variances_as_standard_deviation_of_the_mean(mean_func);
  variances_as_standard_deviation_of_the_mean(nanmean_func);
}

TEST(MeanTest, nanmean_basic) {
  const auto var =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, units::m,
                           Values{1.0, 2.0, 3.0, double(NAN)});
  const auto meanX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.0});
  const auto meanY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 2.0});
  EXPECT_EQ(nanmean(var, Dim::X), meanX);
  EXPECT_EQ(nanmean(var, Dim::Y), meanY);
}

TEST(MeanTest, nanmean_basic_inplace) {
  const auto var =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, units::m,
                           Values{1.0, 2.0, 3.0, double(NAN)});
  auto meanX = makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m);
  auto meanY = makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m);
  const auto expectedX =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, units::m, Values{1.5, 3.0});
  const auto expectedY =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, units::m, Values{2.0, 2.0});
  auto viewX = nanmean(var, Dim::X, meanX);
  auto viewY = nanmean(var, Dim::Y, meanY);
  EXPECT_EQ(meanX, expectedX);
  EXPECT_EQ(viewX, meanX);
  EXPECT_EQ(viewX.underlying(), meanX);
  EXPECT_EQ(meanY, expectedY);
  EXPECT_EQ(viewY, meanY);
  EXPECT_EQ(viewY.underlying(), meanY);
}
