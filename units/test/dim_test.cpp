// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/units/dim.h"

using namespace scipp::units::next;

TEST(DimTest, basics) {
  EXPECT_EQ(Dim(Dim::X), Dim(Dim::X));
  EXPECT_NE(Dim(Dim::X), Dim(Dim::Y));
  EXPECT_EQ(Dim("abc"), Dim("abc"));
  EXPECT_NE(Dim("abc"), Dim("def"));
  EXPECT_EQ(Dim(Dim::X).name(), "Dim.X");
  EXPECT_EQ(Dim("abc").name(), "abc");
}
