// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/indexed_slice_view.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(IndexedSliceViewTest, variable) {
  const auto var = makeVariable<double>({Dim::X, 4}, {1, 2, 3, 4});
  const auto view = IndexedSliceView{var, Dim::X, {2, 2, 0, 3, 1}};
  EXPECT_EQ(view.dim(), Dim::X);
  EXPECT_EQ(view.size(), 5);
  EXPECT_EQ(view[0], var.slice({Dim::X, 2}));
  EXPECT_EQ(view[1], var.slice({Dim::X, 2}));
  EXPECT_EQ(view[2], var.slice({Dim::X, 0}));
  EXPECT_EQ(view[3], var.slice({Dim::X, 3}));
  EXPECT_EQ(view[4], var.slice({Dim::X, 1}));
  EXPECT_EQ(std::distance(view.begin(), view.end()), 5);
  auto begin = view.begin();
  EXPECT_EQ(*begin++, var.slice({Dim::X, 2}));
  EXPECT_EQ(*begin++, var.slice({Dim::X, 2}));
  EXPECT_EQ(*begin++, var.slice({Dim::X, 0}));
  EXPECT_EQ(*begin++, var.slice({Dim::X, 3}));
  EXPECT_EQ(*begin++, var.slice({Dim::X, 1}));
  EXPECT_EQ(begin, view.end());
}
