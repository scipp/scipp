// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/variable/indexed_slice_view.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

TEST(IndexedSliceViewTest, variable) {
  const auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  const auto view = IndexedSliceView{var, Dim::X, {2, 2, 0, 3, 1}};
  EXPECT_EQ(view.dim(), Dim::X);
  EXPECT_EQ(view.size(), 5);
  EXPECT_EQ(view[0], var.slice({Dim::X, 2, 3}));
  EXPECT_EQ(view[1], var.slice({Dim::X, 2, 3}));
  EXPECT_EQ(view[2], var.slice({Dim::X, 0, 1}));
  EXPECT_EQ(view[3], var.slice({Dim::X, 3, 4}));
  EXPECT_EQ(view[4], var.slice({Dim::X, 1, 2}));
  EXPECT_EQ(std::distance(view.begin(), view.end()), 5);
  auto begin = view.begin();
  EXPECT_EQ(*begin++, var.slice({Dim::X, 2, 3}));
  EXPECT_EQ(*begin++, var.slice({Dim::X, 2, 3}));
  EXPECT_EQ(*begin++, var.slice({Dim::X, 0, 1}));
  EXPECT_EQ(*begin++, var.slice({Dim::X, 3, 4}));
  EXPECT_EQ(*begin++, var.slice({Dim::X, 1, 2}));
  EXPECT_EQ(begin, view.end());
}
