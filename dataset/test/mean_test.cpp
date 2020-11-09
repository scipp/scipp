// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "scipp/dataset/reduction.h"
#include "test_macros.h"
#include <gtest/gtest.h>
#include <scipp/common/overloaded.h>

namespace {
using namespace scipp;
using namespace scipp::dataset;

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

TEST(MeanTest, masked_data_array) {
  masked_data_array(mean_func);
  masked_data_array(nanmean_func);
}

TEST(MeanTest, masked_data_array_two_masks) {
  masked_data_array_two_masks(mean_func);
  masked_data_array_two_masks(nanmean_func);
}
