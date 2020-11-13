// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/arg_list.h"

#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;

class TransformBinsTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  Variable var = make_bins(indices, Dim::X, buffer);
};

TEST_F(TransformBinsTest, sandbox) {
  auto view = make_non_owning_typed_bins<double>(var);
  EXPECT_EQ(variableFactory().values<bucket<typed_bin<double>>>(var).size(), 2);
  EXPECT_EQ(variableFactory().values<bucket<typed_bin<double>>>(var)[0].size(),
            2.0);
  EXPECT_EQ(
      variableFactory().values<bucket<typed_bin<double>>>(var)[0].values()[1],
      2.0);

  // Problem: transform bypasses the ElementArrayView iterator and uses data +
  // offset
  transform_in_place<bucket<typed_bin<double>>>(
      view, overloaded{[](const units::Unit &) {},
                       [](const auto &bin) {
                         for (auto x : bin.values())
                           printf("%lf\n", x);
                       }});
  // EXPECT_EQ(view[0].values()[0], 1);
  // EXPECT_EQ(view[1].values()[0], 3);
  // EXPECT_EQ(view[1].values()[1], 4);
}
