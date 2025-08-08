// SPDX-License-Identifier: BSD-3-Clause Copyright (c) 2022 Scipp contributors
// (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/bins.h"
#include "scipp/variable/structures.h"

using namespace scipp;

void check_equal(const Variable &a, const Variable &b) {
  EXPECT_TRUE(equals_nan(a, b));
  EXPECT_TRUE(equals_nan(a, copy(b)));
  EXPECT_NE(a, b);
  EXPECT_NE(a, copy(b));
}

TEST(EqualsNanTest, values) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                  Values{1.0f, 2.0f, NAN, 4.0f});
  check_equal(var, var);
}

TEST(EqualsNanTest, variances) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                  Values{1.0f, 2.0f, 3.0f, 4.0f},
                                  Variances{1.0f, 2.0f, NAN, 4.0f});
  check_equal(var, var);
}

TEST(EqualsNanTest, nested) {
  auto inner = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                    Values{1.0f, 2.0f, NAN, 4.0f});
  auto var = makeVariable<Variable>(Values{inner});
  check_equal(var, var);
}

TEST(EqualsNanTest, structured) {
  auto var =
      variable::make_vectors(Dimensions(Dim::X, 1), sc_units::m, {1, 2, NAN});
  check_equal(var, var);
}

TEST(EqualsNanTest, binned) {
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable buffer = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                         Values{1.0f, 2.0f, NAN, 4.0f});
  Variable var = make_bins(indices, Dim::X, buffer);
  check_equal(var, var);
}
