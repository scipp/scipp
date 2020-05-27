// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/rebin.h"
#include "scipp/variable/variable.h"

using namespace scipp;

TEST(Variable, rebin) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 2.0});
  var.setUnit(units::counts);
  const auto oldEdge =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.0, 2.0, 3.0});
  const auto newEdge =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1.0, 3.0});
  auto rebinned = rebin(var, Dim::X, oldEdge, newEdge);
  ASSERT_EQ(rebinned.dims().shape().size(), 1);
  ASSERT_EQ(rebinned.dims().volume(), 1);
  ASSERT_EQ(rebinned.values<double>().size(), 1);
  EXPECT_EQ(rebinned.values<double>()[0], 3.0);
}
