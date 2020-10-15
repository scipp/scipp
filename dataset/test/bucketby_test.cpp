// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bucketby.h"
#include "scipp/dataset/string.h"

using namespace scipp;
using namespace scipp::dataset;

class DataArrayBucketByTest : public ::testing::Test {
protected:
  Variable data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 3, 2, 4});
  Variable x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{4, 3, 2, 1});
  Variable mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                     Values{true, false, false, false});
  Variable scalar = makeVariable<double>(Values{1.1});
  DataArray table =
      DataArray(data, {{Dim::X, x}, {Dim("scalar"), scalar}}, {{"mask", mask}});
  Variable edges_x =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 2, 4});
};

TEST_F(DataArrayBucketByTest, 1d) {
  Variable sorted_data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{4, 3, 2, 1}, Variances{4, 2, 3, 1});
  Variable sorted_x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4});
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                            Values{false, false, false, true});
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}, {Dim("scalar"), scalar}},
                {{"mask", sorted_mask}});

  EXPECT_EQ(sortby(table, Dim::X), sorted_table);
  const auto bucketed = bucketby(table, Dim::X, edges_x);
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[0],
            sorted_table.slice({Dim::Event, 0, 1}));
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[1],
            sorted_table.slice({Dim::Event, 1, 3}));
}

TEST_F(DataArrayBucketByTest, 2d) {
  Variable edges_y =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{0, 1, 2});
  Variable y =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 1, 2});
  table.coords().set(Dim::Y, y);

  // TODO
  Variable sorted_data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{4, 3, 2, 1}, Variances{4, 2, 3, 1});
  Variable sorted_x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4});
  Variable sorted_y =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{2, 1, 2, 1});
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                            Values{false, false, false, true});
  DataArray sorted_table = DataArray(
      sorted_data,
      {{Dim::X, sorted_x}, {Dim::Y, sorted_y}, {Dim("scalar"), scalar}},
      {{"mask", sorted_mask}});

  const auto bucketed = bucketby(table, Dim::X, edges_x, Dim::Y, edges_y);
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[0],
            sorted_table.slice({Dim::Event, 0, 1}));
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[1],
            sorted_table.slice({Dim::Event, 1, 3}));
}
