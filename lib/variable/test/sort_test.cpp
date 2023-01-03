// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/sort.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

class SortTest : public ::testing::Test {
protected:
  Dimensions dims{{Dim::Y, 2}, {Dim::X, 3}};
  Variable var =
      makeVariable<double>(dims, Values{1.0, 3.0, 2.0, 4.0, 0.0, 5.0},
                           Variances{1.0, 2.0, 3.0, 3.0, 2.0, 1.0});
};

TEST_F(SortTest, ascending) {
  var = values(var);
  auto sorted = sort(var, Dim::X, SortOrder::Ascending);
  EXPECT_NE(sorted, var);
  EXPECT_EQ(sorted,
            makeVariable<double>(dims, Values{1.0, 2.0, 3.0, 0.0, 4.0, 5.0}));
}

TEST_F(SortTest, descending) {
  var = values(var);
  auto sorted = sort(var, Dim::X, SortOrder::Descending);
  EXPECT_NE(sorted, var);
  EXPECT_EQ(sorted,
            makeVariable<double>(dims, Values{3.0, 2.0, 1.0, 5.0, 4.0, 0.0}));
}

TEST_F(SortTest, ascending_with_variances) {
  EXPECT_EQ(sort(var, Dim::X, SortOrder::Ascending),
            makeVariable<double>(dims, Values{1.0, 2.0, 3.0, 0.0, 4.0, 5.0},
                                 Variances{1.0, 3.0, 2.0, 2.0, 3.0, 1.0}));
}

TEST_F(SortTest, descending_with_variances) {
  EXPECT_EQ(sort(var, Dim::X, SortOrder::Descending),
            makeVariable<double>(dims, Values{3.0, 2.0, 1.0, 5.0, 4.0, 0.0},
                                 Variances{2.0, 3.0, 1.0, 1.0, 3.0, 2.0}));
}
