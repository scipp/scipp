// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <scipp/common/overloaded.h>
#include <vector>

#include "test_macros.h"

#include "scipp/common/overloaded.h"
#include "scipp/core/except.h"
#include "scipp/dataset/reduction.h"
#include "scipp/variable/reduction.h"

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

template <typename Op> void masked_data_array(Op op) {
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
  EXPECT_EQ(op(a, Dim::X).data(), meanX);
  EXPECT_EQ(op(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(op(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(op(a, Dim::Y).masks().contains("mask"));
}

template <typename Op> void masked_data_array_two_masks(Op op) {
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
  EXPECT_EQ(op(a, Dim::X).data(), meanX);
  EXPECT_EQ(op(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(op(a, Dim::X).masks().contains("x"));
  EXPECT_TRUE(op(a, Dim::X).masks().contains("y"));
  EXPECT_TRUE(op(a, Dim::Y).masks().contains("x"));
  EXPECT_FALSE(op(a, Dim::Y).masks().contains("y"));
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
               }};
auto nanmean_func = overloaded{
    [](const auto &var, const auto &dim) { return nanmean(var, dim); },
    [](const auto &var, const auto &dim, auto &out) {
      return nanmean(var, dim, out);
    }};
} // namespace

TEST(MeanTest, unknown_dim_fail) {
  unknown_dim_fail(mean_func);
  unknown_dim_fail(nanmean_func);
}

TEST(MeanTest, basic) { basic(mean_func); }
TEST(MeanTest, basic_nan) { basic(nanmean_func); }

TEST(MeanTest, basic_in_place) {
  basic_in_place(mean_func);
  basic_in_place(nanmean_func);
}

TEST(MeanTest, in_place_fail_output_dtype) {
  in_place_fail_output_dtype(mean_func);
  in_place_fail_output_dtype(nanmean_func);
}

TEST(MeanTest, masked_data_array) {
  masked_data_array(mean_func);
  masked_data_array(nanmean_func);
}

TEST(MeanTest, masked_data_array_two_masks) {
  masked_data_array_two_masks(mean_func);
  masked_data_array_two_masks(nanmean_func);
}

TEST(MeanTest, dtype_float_preserved) {
  dtype_float_preserved(mean_func);
  dtype_float_preserved(nanmean_func);
}

TEST(MeanTest, dtype_int_gives_double_mean) {
  dtype_int_gives_double_mean(mean_func);
  EXPECT_THROW(dtype_int_gives_double_mean(nanmean_func),
               except::TypeError); // nansum and nanmean do not support ints
}

TEST(MeanTest, variances_as_standard_deviation_of_the_mean) {
  variances_as_standard_deviation_of_the_mean(mean_func);
  variances_as_standard_deviation_of_the_mean(nanmean_func);
}

TEST(MeanTest, dataset_mean_fails) {
  Dataset d;
  d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{2}));
  d.setData("b", makeVariable<double>(Values{1.0}));
  // "b" does not depend on X, so this fails. This could change in the future if
  // we find a clear definition of the functions behavior in this case.
  EXPECT_THROW(mean(d, Dim::X), except::DimensionError);
}
