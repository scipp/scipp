// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::units;

TEST(DimensionTest, basics) {
  EXPECT_EQ(to_string(dummy::DimId::Invalid), "Dim.Invalid");
  EXPECT_EQ(to_string(dummy::DimId::X), "Dim.X");
  EXPECT_EQ(to_string(dummy::DimId::Y), "Dim.Y");
  EXPECT_EQ(to_string(dummy::DimId::Z), "Dim.Z");
}
