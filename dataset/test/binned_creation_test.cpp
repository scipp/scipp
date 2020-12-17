// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/variable/creation.h"

using namespace scipp;

class BinnedCreationTest : public ::testing::Test {
protected:
  Variable indices = makeVariable<scipp::index_pair>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 5}});
  Variable data =
      makeVariable<double>(Dims{Dim::Event}, Shape{5}, Values{1, 2, 3, 4, 5});
  DataArray buffer = DataArray(data, {{Dim::X, data}});
  Variable var = make_bins(indices, Dim::Event, buffer);
};

TEST_F(BinnedCreationTest, empty_like_default_shape) {
  const auto empty = empty_like(var);
  EXPECT_EQ(empty.dims(), var.dims());
}

TEST_F(BinnedCreationTest, empty_like) {
  Variable shape = makeVariable<scipp::index>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                              Values{1, 2, 5, 6, 3, 4});
  const auto empty = empty_like(var, shape);
  EXPECT_EQ(empty.dims(), shape.dims());
  const auto [indices, dim, buf] = empty.constituents<core::bin<DataArray>>();
  EXPECT_EQ(buf.dims(), Dimensions(Dim::Event, 21));
  scipp::index i = 0;
  for (const auto n : {1, 2, 5, 6, 3, 4}) {
    EXPECT_EQ(empty.values<core::bin<DataArray>>()[i++].dims()[Dim::Event], n);
  }
}
