// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::units;

TEST(DimensionTest, basics) {
  EXPECT_EQ(simple::to_string(simple::Dim::Invalid), "Dim::Invalid");
  EXPECT_EQ(simple::to_string(simple::Dim::X), "Dim::X");
  EXPECT_EQ(simple::to_string(simple::Dim::Y), "Dim::Y");
  EXPECT_EQ(simple::to_string(simple::Dim::Z), "Dim::Z");
}
