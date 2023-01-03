// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/variable/reduction.h"
#include "test_macros.h"

using namespace scipp;

TEST(ReduceLogicalTest, fails) {
  const auto bad = makeVariable<int32_t>(Dims{Dim::X}, Shape{2});
  EXPECT_THROW_DISCARD(all(bad, Dim::X), except::TypeError);
  EXPECT_THROW_DISCARD(any(bad, Dim::X), except::TypeError);
  EXPECT_THROW_DISCARD(all(bad, Dim::Y), except::DimensionError);
  EXPECT_THROW_DISCARD(any(bad, Dim::Y), except::DimensionError);
}

TEST(ReduceLogicalTest, all) {
  const auto var = makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                      Values{true, true, false, true});
  EXPECT_EQ(all(var, Dim::X),
            makeVariable<bool>(Dims{Dim::Y}, Shape{2}, Values{false, true}));
  EXPECT_EQ(all(var, Dim::Y),
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{true, false}));
  EXPECT_EQ(all(var.slice({Dim::X, 0, 0}), Dim::X),
            makeVariable<bool>(Dims{Dim::Y}, Shape{2}, Values{true, true}));
}

TEST(ReduceLogicalTest, any) {
  const auto var = makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                      Values{false, false, true, false});
  EXPECT_EQ(any(var, Dim::X),
            makeVariable<bool>(Dims{Dim::Y}, Shape{2}, Values{true, false}));
  EXPECT_EQ(any(var, Dim::Y),
            makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, true}));
  EXPECT_EQ(any(var.slice({Dim::X, 0, 0}), Dim::X),
            makeVariable<bool>(Dims{Dim::Y}, Shape{2}, Values{false, false}));
}
