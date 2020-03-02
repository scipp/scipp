// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/axis.h"

using namespace scipp;
using namespace scipp::core;

TEST(AxisTest, hasUnaligned) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{4});
  DatasetAxis axis(var);
  EXPECT_FALSE(axis.hasUnaligned());
  EXPECT_FALSE(axis["a"].hasUnaligned());
  EXPECT_FALSE(DatasetAxisConstView(axis).hasUnaligned());
  axis.unaligned().set("a", var);
  EXPECT_TRUE(axis.hasUnaligned());
  EXPECT_TRUE(axis["a"].hasUnaligned());
  EXPECT_FALSE(axis["b"].hasUnaligned());
  EXPECT_TRUE(DatasetAxisConstView(axis).hasUnaligned());
}
