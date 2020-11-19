// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "random.h"

#include "scipp/dataset/bins.h"
#include "scipp/dataset/bucketby.h"
#include "scipp/dataset/string.h"
#include "scipp/variable/arithmetic.h"

using namespace scipp;
using namespace scipp::dataset;

class DataArrayBucketByTest : public ::testing::Test {
protected:
  Variable data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4}, Variances{1, 3, 2, 4});
  Variable x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{3, 2, 4, 1});
  Variable mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                     Values{true, false, false, false});
  Variable scalar = makeVariable<double>(Values{1.1});
  DataArray table =
      DataArray(data, {{Dim::X, x}, {Dim("scalar"), scalar}}, {{"mask", mask}});
  Variable edges_x =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 2, 4});
};

TEST_F(DataArrayBucketByTest, sort_1d) {
  Variable sorted_data = makeVariable<double>(
      Dims{Dim::Event}, Shape{4}, Values{4, 2, 1, 3}, Variances{4, 3, 1, 2});
  Variable sorted_x =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4});
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                            Values{false, false, true, false});
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}, {Dim("scalar"), scalar}},
                {{"mask", sorted_mask}});
  EXPECT_EQ(sortby(table, Dim::X), sorted_table);
}

TEST_F(DataArrayBucketByTest, 1d) {
  Variable sorted_data = makeVariable<double>(
      Dims{Dim::Event}, Shape{3}, Values{4, 1, 2}, Variances{4, 1, 3});
  Variable sorted_x =
      makeVariable<double>(Dims{Dim::Event}, Shape{3}, Values{1, 3, 2});
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{3},
                                            Values{false, true, false});
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}, {Dim("scalar"), scalar}},
                {{"mask", sorted_mask}});

  const auto bucketed = bucketby(table, {edges_x});

  EXPECT_EQ(bucketed.dims(), Dimensions(Dim::X, 2));
  EXPECT_EQ(bucketed.coords()[Dim::X], edges_x);
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[0],
            sorted_table.slice({Dim::Event, 0, 1}));
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[1],
            sorted_table.slice({Dim::Event, 1, 3}));
}

TEST_F(DataArrayBucketByTest, 2d) {
  Variable edges_y =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{0, 1, 3});
  Variable y =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 1, 2});
  table.coords().set(Dim::Y, y);

  Variable sorted_data = makeVariable<double>(
      Dims{Dim::Event}, Shape{3}, Values{4, 1, 2}, Variances{4, 1, 3});
  Variable sorted_x =
      makeVariable<double>(Dims{Dim::Event}, Shape{3}, Values{1, 3, 2});
  Variable sorted_y =
      makeVariable<double>(Dims{Dim::Event}, Shape{3}, Values{2, 1, 2});
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{3},
                                            Values{false, true, false});
  DataArray sorted_table = DataArray(
      sorted_data,
      {{Dim::X, sorted_x}, {Dim::Y, sorted_y}, {Dim("scalar"), scalar}},
      {{"mask", sorted_mask}});

  const auto bucketed = bucketby(table, {edges_x, edges_y});

  EXPECT_EQ(bucketed.dims(), Dimensions({Dim::X, Dim::Y}, {2, 2}));
  EXPECT_EQ(bucketed.coords()[Dim::X], edges_x);
  EXPECT_EQ(bucketed.coords()[Dim::Y], edges_y);
  const auto empty_bucket = sorted_table.slice({Dim::Event, 0, 0});
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[0], empty_bucket);
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[1],
            sorted_table.slice({Dim::Event, 0, 1}));
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[2], empty_bucket);
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[3],
            sorted_table.slice({Dim::Event, 1, 3}));

  EXPECT_EQ(bucketby(bucketby(table, {edges_x}), {edges_y}), bucketed);
}

class BinTest : public ::testing::Test {
protected:
  auto make_table(const scipp::index size) {
    Random rand;
    rand.seed(0);
    const Dimensions dims(Dim::Row, size);

    const auto data =
        makeVariable<double>(Dimensions{dims}, Values(rand(dims.volume())));
    const auto x =
        makeVariable<double>(Dimensions{dims}, Values(rand(dims.volume())));
    const auto y =
        makeVariable<double>(Dimensions{dims}, Values(rand(dims.volume())));
    return DataArray(data, {{Dim::X, x}, {Dim::Y, y}});
  }
  Variable edges_x =
      makeVariable<double>(Dims{Dim::X}, Shape{5}, Values{-2, -1, 0, 1, 2});
  Variable edges_y =
      makeVariable<double>(Dims{Dim::Y}, Shape{5}, Values{-2, -1, 0, 1, 2});
  Variable edges_x_coarse =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{-2, 1, 2});
  Variable edges_y_coarse =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{-2, -1, 2});

  auto expect_near(const DataArrayConstView &a, const DataArrayConstView &b,
                   const double scale = 100.0) {
    // cut off last digits for approximate floating point comparison
    const auto truncate = scale * units::one;
    EXPECT_EQ(buckets::sum(a) + truncate, buckets::sum(b) + truncate);
  }
};

TEST_F(BinTest, rebin_coarse_to_fine_1d) {
  const auto table = make_table(30);
  EXPECT_EQ(bucketby(table, {edges_x}),
            bucketby(bucketby(table, {edges_x_coarse}), {edges_x}));
}

TEST_F(BinTest, rebin_fine_to_coarse_1d) {
  const auto table = make_table(30);
  expect_near(bucketby(table, {edges_x_coarse}),
              bucketby(bucketby(table, {edges_x}), {edges_x_coarse}));
}

TEST_F(BinTest, 2d) {
  const auto table = make_table(30);
  const auto x = bucketby(table, {edges_x});
  const auto x_then_y = bucketby(x, {edges_y});
  const auto xy = bucketby(table, {edges_x, edges_y});
  EXPECT_EQ(xy, x_then_y);
}

TEST_F(BinTest, rebin_coarse_to_fine_2d) {
  const auto table = make_table(30);
  const auto xy_coarse = bucketby(table, {edges_x_coarse, edges_y_coarse});
  const auto xy = bucketby(table, {edges_x, edges_y});
  EXPECT_EQ(bucketby(xy_coarse, {edges_x, edges_y}), xy);
}

TEST_F(BinTest, rebin_fine_to_coarse_2d) {
  const auto table = make_table(30);
  const auto xy_coarse = bucketby(table, {edges_x_coarse, edges_y_coarse});
  const auto xy = bucketby(table, {edges_x, edges_y});
  expect_near(bucketby(xy, {edges_x_coarse, edges_y_coarse}), xy_coarse, 200.0);
}

TEST_F(BinTest, rebin_coarse_to_fine_2d_inner) {
  const auto table = make_table(30);
  const auto xy_coarse = bucketby(table, {edges_x_coarse, edges_y_coarse});
  const auto xy = bucketby(table, {edges_x_coarse, edges_y});
  expect_near(bucketby(xy_coarse, {edges_y}), xy);
}

TEST_F(BinTest, rebin_coarse_to_fine_2d_outer) {
  const auto table = make_table(30);
  const auto xy_coarse = bucketby(table, {edges_x_coarse, edges_y});
  const auto xy = bucketby(table, {edges_x, edges_y});
  expect_near(bucketby(xy_coarse, {edges_x}), xy);
}
