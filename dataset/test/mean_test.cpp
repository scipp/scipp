// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "scipp/dataset/reduction.h"
#include "test_macros.h"
#include <gtest/gtest.h>
#include <scipp/common/overloaded.h>
#include "scipp/dataset/string.h"

namespace {
using namespace scipp;
using namespace scipp::dataset;

template <typename Op> void test_masked_data_array_1_mask(Op op) {
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
  std::cout << to_string(op(a, Dim::X).data()) << std::endl;
  EXPECT_EQ(op(a, Dim::X).data(), meanX);
  EXPECT_EQ(op(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(op(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(op(a, Dim::Y).masks().contains("mask"));
}

template <typename Op> void test_masked_data_array_2_masks(Op op) {
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

template <typename Op> void test_masked_data_array_nd_mask(Op op) {
  const auto var = makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                        units::m, Values{1.0, 2.0, 3.0, 4.0});
  // Just a single masked element
  const auto mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                       Values{false, true, false, false});
  DataArray a(var);
  a.masks().set("mask", mask);
  const auto meanX =
      makeVariable<double>(Dimensions{Dim::Y, 2}, units::m,
                           Values{(1.0 + 0.0) / 1, (3.0 + 4.0) / 2});
  const auto meanY =
      makeVariable<double>(Dimensions{Dim::X, 2}, units::m,
                           Values{(1.0 + 3.0) / 2, (0.0 + 4.0) / 1});
  const auto mean = makeVariable<double>(units::m, Shape{1},
                                         Values{(1.0 + 0.0 + 3.0 + 4.0) / 3});
  EXPECT_EQ(op(a, Dim::X).data(), meanX);
  EXPECT_EQ(op(a, Dim::Y).data(), meanY);
  EXPECT_EQ(op(a).data(), mean);
  EXPECT_FALSE(op(a, Dim::X).masks().contains("mask"));
  EXPECT_FALSE(op(a, Dim::X).masks().contains("mask"));
  EXPECT_FALSE(op(a).masks().contains("mask"));
}

auto mean_func = overloaded{
    [](const auto &x, const auto &dim) { return mean(x, dim); },
    [](const auto &x, const auto &dim, auto &out) { return mean(x, dim, out); },
    [](const auto &x) { return mean(x); }};
auto nanmean_func =
    overloaded{[](const auto &x, const auto &dim) { return nanmean(x, dim); },
               [](const auto &x, const auto &dim, auto &out) {
                 return nanmean(x, dim, out);
               },
               [](const auto &x) { return nanmean(x); }};
} // namespace

TEST(MeanTest, masked_data_array) {
  test_masked_data_array_1_mask(mean_func);
  test_masked_data_array_1_mask(nanmean_func);
}

TEST(MeanTest, masked_data_array_two_masks) {
  test_masked_data_array_2_masks(mean_func);
  test_masked_data_array_2_masks(nanmean_func);
}

TEST(MeanTest, masked_data_array_md_masks) {
  test_masked_data_array_nd_mask(mean_func);
  test_masked_data_array_nd_mask(nanmean_func);
}

TEST(MeanTest, nanmean_masked_data_with_nans) {
  // Two Nans
  const auto var =
      makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, units::m,
                           Values{double(NAN), double(NAN), 3.0, 4.0});
  // Two masked element
  const auto mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                       Values{false, true, true, false});
  DataArray a(var);
  a.masks().set("mask", mask);
  // First element NaN, second NaN AND masked, third masked, forth non-masked
  // finite number
  const auto mean = makeVariable<double>(units::m, Shape{1},
                                         Values{(0.0 + 0.0 + 0.0 + 4.0) / 1});
  EXPECT_EQ(nanmean(a).data(), mean);
}
