// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "dataset_test_common.h"

#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/string.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/math.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::dataset;
using testdata::make_table;

class DataArrayBinTest : public ::testing::Test {
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

TEST_F(DataArrayBinTest, 1d) {
  Variable sorted_data = makeVariable<double>(
      Dims{Dim::Event}, Shape{3}, Values{4, 1, 2}, Variances{4, 1, 3});
  Variable sorted_x =
      makeVariable<double>(Dims{Dim::Event}, Shape{3}, Values{1, 3, 2});
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{3},
                                            Values{false, true, false});
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}}, {{"mask", sorted_mask}});

  const auto bucketed = bin(table, {edges_x});

  EXPECT_EQ(bucketed.dims(), Dimensions(Dim::X, 2));
  EXPECT_EQ(bucketed.coords()[Dim::X], edges_x);
  EXPECT_EQ(bucketed.coords()[Dim("scalar")], scalar);
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[0],
            sorted_table.slice({Dim::Event, 0, 1}));
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[1],
            sorted_table.slice({Dim::Event, 1, 3}));
}

TEST_F(DataArrayBinTest, 2d) {
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
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}, {Dim::Y, sorted_y}},
                {{"mask", sorted_mask}});

  const auto bucketed = bin(table, {edges_x, edges_y});

  EXPECT_EQ(bucketed.dims(), Dimensions({Dim::X, Dim::Y}, {2, 2}));
  EXPECT_EQ(bucketed.coords()[Dim::X], edges_x);
  EXPECT_EQ(bucketed.coords()[Dim::Y], edges_y);
  EXPECT_EQ(bucketed.coords()[Dim("scalar")], scalar);
  const auto empty_bucket = sorted_table.slice({Dim::Event, 0, 0});
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[0], empty_bucket);
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[1],
            sorted_table.slice({Dim::Event, 0, 1}));
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[2], empty_bucket);
  EXPECT_EQ(bucketed.values<bucket<DataArray>>()[3],
            sorted_table.slice({Dim::Event, 1, 3}));

  EXPECT_EQ(bin(bin(table, {edges_x}), {edges_y}), bucketed);
}

TEST_F(DataArrayBinTest, operations_on_empty) {
  const Variable indices = makeVariable<std::pair<scipp::index, scipp::index>>(
      Dimensions{{Dim::Event, 0}, {Dim::Y, 0}}, Values{});
  const Variable binned = make_bins(indices, Dim::Event, data);

  EXPECT_EQ(abs(binned), binned);
  EXPECT_EQ(binned, binned * binned);
  EXPECT_EQ(binned, binned * (2 * units::one));
}

TEST(BinGroupTest, 1d) {
  const Dimensions dims(Dim::Row, 5);
  const auto data = makeVariable<double>(dims, Values{1, 2, 3, 4, 5});
  const auto label =
      makeVariable<std::string>(dims, Values{"a", "b", "c", "b", "a"});
  const auto table = DataArray(data, {{Dim("label"), label}});
  Variable groups =
      makeVariable<std::string>(Dims{Dim("label")}, Shape{2}, Values{"a", "c"});
  const auto binned = bin(table, {}, {groups});
  EXPECT_EQ(binned.dims(), groups.dims());
  EXPECT_EQ(binned.values<core::bin<DataArray>>()[1],
            table.slice({Dim::Row, 2, 3}));
  EXPECT_EQ(binned.values<core::bin<DataArray>>()[0].slice({Dim::Row, 0}),
            table.slice({Dim::Row, 0}));
  EXPECT_EQ(binned.values<core::bin<DataArray>>()[0].slice({Dim::Row, 1}),
            table.slice({Dim::Row, 4}));
}

class BinTest : public ::testing::TestWithParam<DataArray> {
protected:
  Variable groups = makeVariable<int64_t>(Dims{Dim("group")}, Shape{5},
                                          Values{-2, -1, 0, 1, 2});
  Variable groups2 = makeVariable<int64_t>(Dims{Dim("group2")}, Shape{5},
                                           Values{-2, -1, 0, 1, 2});
  Variable edges_x =
      makeVariable<double>(Dims{Dim::X}, Shape{5}, Values{-2, -1, 0, 1, 2});
  Variable edges_y =
      makeVariable<double>(Dims{Dim::Y}, Shape{5}, Values{-2, -1, 0, 1, 2});
  Variable edges_x_coarse =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{-2, 1, 2});
  Variable edges_y_coarse =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{-2, -1, 2});

  void expect_near(const DataArrayConstView &a, const DataArrayConstView &b) {
    const auto tolerance = max(buckets::sum(a.data())) * (1e-14 * units::one);
    EXPECT_TRUE(all(is_approx(buckets::sum(a.data()), buckets::sum(b.data()),
                              tolerance))
                    .value<bool>());
    EXPECT_EQ(a.masks(), b.masks());
    EXPECT_EQ(a.coords(), b.coords());
    EXPECT_EQ(a.attrs(), b.attrs());
  }
};

INSTANTIATE_TEST_SUITE_P(InputSize, BinTest,
                         testing::Values(make_table(0), make_table(1),
                                         make_table(7), make_table(27),
                                         make_table(1233)));

TEST_P(BinTest, group) {
  const auto table = GetParam();
  const auto binned = bin(table, {}, {groups});
  EXPECT_EQ(binned.dims(), groups.dims());
}

TEST_P(BinTest, no_edges_or_groups) {
  const auto table = GetParam();
  EXPECT_THROW(bin(table, {}), except::BucketError);
}

TEST_P(BinTest, edges_too_short) {
  const auto table = GetParam();
  const auto edges = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1});
  EXPECT_THROW(bin(table, {edges}), except::BucketError);
}

TEST_P(BinTest, rebin_coarse_to_fine_1d) {
  const auto table = GetParam();
  EXPECT_EQ(bin(table, {edges_x}),
            bin(bin(table, {edges_x_coarse}), {edges_x}));
}

TEST_P(BinTest, rebin_fine_to_coarse_1d) {
  const auto table = GetParam();
  expect_near(bin(table, {edges_x_coarse}),
              bin(bin(table, {edges_x}), {edges_x_coarse}));
}

TEST_P(BinTest, 2d) {
  const auto table = GetParam();
  const auto x = bin(table, {edges_x});
  const auto x_then_y = bin(x, {edges_y});
  const auto xy = bin(table, {edges_x, edges_y});
  EXPECT_EQ(xy, x_then_y);
}

TEST_P(BinTest, 2d_drop_out_of_range_linspace) {
  const auto edges_x_drop = edges_x.slice({Dim::X, 1, 4});
  const auto edges_y_drop = edges_y.slice({Dim::Y, 1, 4});
  const auto table = GetParam();
  const auto x_then_y = bin(bin(table, {edges_x_drop}), {edges_y_drop});
  const auto xy = bin(table, {edges_x_drop, edges_y_drop});
  EXPECT_EQ(xy, x_then_y);
}

TEST_P(BinTest, 2d_drop_out_of_range) {
  auto edges_x_drop = edges_x.slice({Dim::X, 1, 4});
  edges_x_drop.values<double>()[0] += 0.001;
  auto edges_y_drop = edges_y.slice({Dim::Y, 1, 4});
  edges_y_drop.values<double>()[0] += 0.001;
  const auto table = GetParam();
  const auto x_then_y = bin(bin(table, {edges_x_drop}), {edges_y_drop});
  const auto xy = bin(table, {edges_x_drop, edges_y_drop});
  EXPECT_EQ(xy, x_then_y);
}

TEST_P(BinTest, 2d_drop_out_of_group) {
  auto groups1_drop = groups.slice({Dim("group"), 1, 4});
  auto groups2_drop = groups2.slice({Dim("group2"), 1, 3});
  const auto table = GetParam();
  EXPECT_EQ(bin(bin(table, {}, {groups1_drop}), {}, {groups2_drop}),
            bin(table, {}, {groups1_drop, groups2_drop}));
}

TEST_P(BinTest, rebin_2d_with_2d_coord) {
  auto table = GetParam();
  auto xy = bin(table, {edges_x_coarse, edges_y_coarse});
  Variable edges_y_2d = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                             Values{-2, 1, 2, -1, 0, 3});
  xy.coords().set(Dim::Y, edges_y_2d);
  auto bins_y = bins_view<DataArray>(xy.data()).coords()[Dim::Y];
  bins_y += 0.5 * units::one;
  EXPECT_THROW(bin(xy, {edges_x_coarse}), except::DimensionError);
  if (table.dims().volume() > 0) {
    EXPECT_NE(bin(xy, {edges_x_coarse, edges_y_coarse}),
              bin(table, {edges_x_coarse, edges_y_coarse}));
    EXPECT_NE(bin(xy, {edges_x, edges_y_coarse}),
              bin(table, {edges_x, edges_y_coarse}));
  }
  table.coords()[Dim::Y] += 0.5 * units::one;
  expect_near(bin(xy, {edges_x_coarse, edges_y_coarse}),
              bin(table, {edges_x_coarse, edges_y_coarse}));
  expect_near(bin(xy, {edges_x, edges_y_coarse}),
              bin(table, {edges_x, edges_y_coarse}));
}

TEST_P(BinTest, rebin_coarse_to_fine_2d) {
  const auto table = GetParam();
  const auto xy_coarse = bin(table, {edges_x_coarse, edges_y_coarse});
  const auto xy = bin(table, {edges_x, edges_y});
  EXPECT_EQ(bin(xy_coarse, {edges_x, edges_y}), xy);
}

TEST_P(BinTest, rebin_fine_to_coarse_2d) {
  const auto table = GetParam();
  const auto xy_coarse = bin(table, {edges_x_coarse, edges_y_coarse});
  const auto xy = bin(table, {edges_x, edges_y});
  expect_near(bin(xy, {edges_x_coarse, edges_y_coarse}), xy_coarse);
}

TEST_P(BinTest, rebin_coarse_to_fine_2d_inner) {
  const auto table = GetParam();
  const auto xy_coarse = bin(table, {edges_x_coarse, edges_y_coarse});
  const auto xy = bin(table, {edges_x_coarse, edges_y});
  expect_near(bin(xy_coarse, {edges_y}), xy);
}

TEST_P(BinTest, rebin_coarse_to_fine_2d_outer) {
  const auto table = GetParam();
  auto xy_coarse = bin(table, {edges_x_coarse, edges_y});
  auto xy = bin(table, {edges_x, edges_y});
  expect_near(bin(xy_coarse, {edges_x}), xy);
  // Y is inside X and needs to be handled by `bin`, but coord is not required.
  xy_coarse.coords().erase(Dim::Y);
  xy.coords().erase(Dim::Y);
  expect_near(bin(xy_coarse, {edges_x}), xy);
}

TEST_P(BinTest, group_and_bin) {
  const auto table = GetParam();
  const auto x_group = bin(table, {edges_x}, {groups});
  const auto group = bin(table, {}, {groups});
  EXPECT_EQ(bin(group, {edges_x}, {}), x_group);
}

TEST_P(BinTest, rebin_masked) {
  const auto table = GetParam();
  auto binned = bin(table, {edges_x_coarse});
  binned.masks().set("x-mask", makeVariable<bool>(Dims{Dim::X}, Shape{2},
                                                  Values{false, true}));
  EXPECT_EQ(buckets::sum(bin(binned, {edges_x})), histogram(binned, edges_x));
  if (table.dims().volume() > 0) {
    EXPECT_NE(bin(binned, {edges_x}), bin(table, {edges_x}));
    EXPECT_NE(buckets::sum(bin(binned, {edges_x})), histogram(table, edges_x));
    binned.masks().erase("x-mask");
    EXPECT_EQ(bin(binned, {edges_x}), bin(table, {edges_x}));
    EXPECT_EQ(buckets::sum(bin(binned, {edges_x})), histogram(table, edges_x));
  }
}

TEST_P(BinTest, unrelated_masks_preserved) {
  const auto table = GetParam();
  auto binned = bin(table, {edges_x_coarse});
  auto expected = bin(table, {edges_x});
  const auto mask = makeVariable<bool>(Values{true});
  binned.masks().set("scalar-mask", mask);
  expected.masks().set("scalar-mask", mask);
  EXPECT_EQ(bin(binned, {edges_x}), expected);
}

TEST_P(BinTest, rebinned_meta_data_dropped) {
  const auto table = GetParam();
  // Same *length* but different edge *position*
  Variable edges_x_coarse2 =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{-2, 0, 2});
  Variable edges_y_coarse2 =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{-2, 0, 2});
  auto xy1 = bin(table, {edges_x_coarse, edges_y_coarse});
  auto xy2 = bin(table, {edges_x_coarse2, edges_y_coarse2});
  expect_near(bin(xy1, {edges_x_coarse2, edges_y_coarse2}), xy2);
  const auto mask_x =
      makeVariable<bool>(Dims{Dim::X}, Shape{2}, Values{false, false});
  xy1.masks().set("x", mask_x);
  xy1.coords().set(Dim("aux1"), mask_x);
  xy1.coords().set(Dim("aux1-edge"), edges_x_coarse);
  xy1.attrs().set(Dim("aux2"), mask_x);
  expect_near(bin(xy1, {edges_x_coarse2, edges_y_coarse2}), xy2);
}

TEST_P(BinTest, bin_by_group) {
  const auto table = GetParam();
  auto binned = bin(table, {}, {groups});
  // Currently `bin` is not removing coords used for grouping, see TODO.
  std::get<2>(binned.data().constituents<core::bin<DataArray>>())
      .coords()
      .erase(Dim("group"));
  const auto edges =
      makeVariable<double>(Dims{Dim("group")}, Shape{3}, Values{-2, 0, 2});
  // Using bin coord )instead of event coord) for binning.
  bin(binned, {edges});
}
