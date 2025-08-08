// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <limits>
#include <vector>

#include "scipp/dataset/all.h"
#include "scipp/dataset/any.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(AllTest, masked_elements_are_ignored) {
  const auto var =
      makeVariable<bool>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
                         Values{true, false, true, true, false, false});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  const auto all_x = makeVariable<bool>(Dimensions{Dim::Y, 3}, sc_units::m,
                                        Values{true, true, false});
  const auto all_y = makeVariable<bool>(Dimensions{Dim::X, 2}, sc_units::m,
                                        Values{false, false});
  EXPECT_EQ(all(a, Dim::X).data(), all_x);
  EXPECT_EQ(all(a, Dim::Y).data(), all_y);
}

TEST(AllTest, mask_along_reduction_dim_is_dropped) {
  const auto var =
      makeVariable<bool>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
                         Values{true, false, true, true, false, false});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  EXPECT_TRUE(all(a, Dim::Y).masks().contains("mask"));
}

TEST(AllTest, mask_along_other_dim_is_kept) {
  const auto var =
      makeVariable<bool>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
                         Values{true, false, true, true, false, false});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  EXPECT_TRUE(all(a, Dim::Y).masks().contains("mask"));
}

TEST(AnyTest, masked_elements_are_ignored) {
  const auto var =
      makeVariable<bool>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
                         Values{false, true, true, true, false, false});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  const auto any_x = makeVariable<bool>(Dimensions{Dim::Y, 3}, sc_units::m,
                                        Values{false, true, false});
  const auto any_y = makeVariable<bool>(Dimensions{Dim::X, 2}, sc_units::m,
                                        Values{true, true});
  EXPECT_EQ(any(a, Dim::X).data(), any_x);
  EXPECT_EQ(any(a, Dim::Y).data(), any_y);
}

TEST(AnyTest, mask_along_reduction_dim_is_dropped) {
  const auto var =
      makeVariable<bool>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
                         Values{false, true, true, true, false, false});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  EXPECT_FALSE(any(a, Dim::X).masks().contains("mask"));
}

TEST(AnyTest, mask_along_other_dim_is_kept) {
  const auto var =
      makeVariable<bool>(Dimensions{{Dim::Y, 3}, {Dim::X, 2}}, sc_units::m,
                         Values{false, true, true, true, false, false});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);

  EXPECT_TRUE(any(a, Dim::Y).masks().contains("mask"));
}
