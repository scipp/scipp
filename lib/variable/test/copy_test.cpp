// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "test_macros.h"

#include "scipp/core/except.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_concept.h"

using namespace scipp;

class CopyTest : public ::testing::Test {
protected:
  void check_copied(const Variable &a, const Variable &b) {
    EXPECT_EQ(a, b);
    EXPECT_NE(&a.dims(), &b.dims());
    EXPECT_NE(&a.unit(), &b.unit());
    EXPECT_NE(a.values<double>().data(), b.values<double>().data());
    if (a.has_variances()) {
      EXPECT_NE(a.variances<double>().data(), b.variances<double>().data());
    }
  }

  Variable xy =
      makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 3}, sc_units::m,
                           Values{1, 2, 3, 4, 5, 6, 7, 8, 9},
                           Variances{10, 11, 12, 13, 14, 15, 16, 17, 18});
};

TEST_F(CopyTest, full) {
  const auto copied = copy(xy);
  check_copied(copied, xy);
  EXPECT_TRUE(equals(copied.strides(), xy.strides()));
  EXPECT_EQ(copied.offset(), 0);
  EXPECT_EQ(copied.data().size(), 9);
}

TEST_F(CopyTest, drops_readonly) {
  const auto readonly = xy.as_const();
  EXPECT_TRUE(readonly.is_readonly());
  EXPECT_FALSE(copy(readonly).is_readonly());
}

TEST_F(CopyTest, slice) {
  const auto sliced = xy.slice({Dim::X, 1, 2}).slice({Dim::Y, 1, 3});
  const auto copied = copy(sliced);
  check_copied(copied, sliced);
  EXPECT_FALSE(equals(copied.strides(), sliced.strides()));
  EXPECT_NE(copied.offset(), sliced.offset());
  EXPECT_EQ(copied.offset(), 0);
  EXPECT_EQ(copied.data().size(), 2);
}

TEST_F(CopyTest, broadcast) {
  const auto var = broadcast(1.2 * sc_units::m, Dimensions(Dim::X, 3));
  const auto copied = copy(var);
  check_copied(copied, var);
  EXPECT_FALSE(equals(copied.strides(), var.strides()));
  EXPECT_EQ(copied.offset(), 0);
  EXPECT_NE(copied.data().size(), var.data().size());
  EXPECT_EQ(copied.data().size(), 3);
}

TEST_F(CopyTest, transpose) {
  const auto var = transpose(xy);
  const auto copied = copy(var);
  check_copied(copied, var);
  EXPECT_FALSE(equals(copied.strides(), var.strides()));
  EXPECT_EQ(copied.offset(), 0);
  EXPECT_EQ(copied.data().size(), 9);
}

TEST_F(CopyTest, broadcast_transpose_slice) {
  const auto sliced = xy.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 3});
  Dimensions dims({Dim::X, Dim::Z, Dim::Y}, {2, 2, 2});
  const auto var = transpose(broadcast(sliced, dims));
  const auto copied = copy(var);
  check_copied(copied, var);
  EXPECT_FALSE(equals(copied.strides(), var.strides()));
  EXPECT_EQ(copied.offset(), 0);
  EXPECT_EQ(copied.data().size(), 8);
  EXPECT_TRUE(equals(var.values<double>(), {5, 8, 5, 8, 6, 9, 6, 9}));
}
