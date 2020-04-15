// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

class VariableConceptTest : public ::testing::Test {
protected:
  const Variable a{makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2})};
  const Variable b{makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2})};
};

TEST_F(VariableConceptTest, isSame_no_slice) {
  const VariableConstView a_view(a);
  const VariableConstView a_view2(a);
  const VariableConstView b_view(b);

  EXPECT_TRUE(a.data().isSame(a.data()));
  EXPECT_FALSE(a.data().isSame(b.data()));

  EXPECT_TRUE(a.data().isSame(a_view.data()));
  EXPECT_TRUE(a_view.data().isSame(a.data()));

  EXPECT_TRUE(a_view.data().isSame(a_view.data()));
  EXPECT_TRUE(a_view.data().isSame(a_view2.data()));
  EXPECT_FALSE(a_view.data().isSame(b_view.data()));
}

TEST_F(VariableConceptTest, isSame_same_slice) {
  const VariableConstView a_view = a.slice({Dim::X, 0, 2});
  const VariableConstView a_view2 = a.slice({Dim::X, 0, 2});
  const VariableConstView b_view = b.slice({Dim::X, 0, 2});

  EXPECT_TRUE(a.data().isSame(a.data()));
  EXPECT_FALSE(a.data().isSame(b.data()));

  // Comparing full slice gives false, even though it could technically be true.
  // This is not an issue for how `isSame` is used. The best way to fix this
  // would be to ignore slicing that has no effect in the creation of variable
  // proxies (and similar for data array proxies and dataset proxies).
  EXPECT_FALSE(a.data().isSame(a_view.data()));
  EXPECT_FALSE(a_view.data().isSame(a.data()));

  EXPECT_TRUE(a_view.data().isSame(a_view.data()));
  EXPECT_TRUE(a_view.data().isSame(a_view2.data()));
  EXPECT_FALSE(a_view.data().isSame(b_view.data()));
}

TEST_F(VariableConceptTest, isSame_different_slice) {
  const VariableConstView a_view1 = a.slice({Dim::X, 0, 1});
  const VariableConstView a_view2 = a.slice({Dim::X, 1, 2});

  EXPECT_FALSE(a_view1.data().isSame(a_view2.data()));
}
