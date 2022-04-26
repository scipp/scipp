// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "dataset_test_common.h"

#include "scipp/dataset/bin.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/bins_view.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/string.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/misc_operations.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"

using namespace scipp;
using namespace scipp::dataset;
using testdata::make_table;

template <class Coords> class DataArrayBinTest : public ::testing::Test {
protected:
  template <size_t I, class... Vals> auto make_coord(Dim dim, Vals &&... vals) {
    using Coord = std::tuple_element_t<I, Coords>;
    return makeVariable<Coord>(Dims{dim}, Shape{sizeof...(vals)},
                               Values{static_cast<Coord>(vals)...});
  }

  Variable data =
      makeVariable<double>(Dims{Dim::Event}, Shape{4}, Values{1, 2, 3, 4},
                           Variances{1, 3, 2, 4}, units::m);
  Variable x = make_coord<0>(Dim::Event, 3, 2, 4, 1);
  Variable y = make_coord<1>(Dim::Event, 1, 2, 1, 2);
  Variable mask = makeVariable<bool>(Dims{Dim::Event}, Shape{4},
                                     Values{true, false, false, false});
  Variable scalar = makeVariable<double>(Values{1.1}, units::kg);
  DataArray table =
      DataArray(data, {{Dim::X, x}, {Dim("scalar"), scalar}}, {{"mask", mask}});
  Variable edges_x = make_coord<0>(Dim::X, 0, 2, 4);
  Variable edges_y = make_coord<1>(Dim::Y, 0, 1, 3);
};

// dtypes of the coordinates
using DataArrayBinTestTypes =
    ::testing::Types<std::tuple<double, double>, std::tuple<float, float>,
                     std::tuple<core::time_point, double>,
                     std::tuple<double, core::time_point>>;

TYPED_TEST_SUITE(DataArrayBinTest, DataArrayBinTestTypes);

TYPED_TEST(DataArrayBinTest, 1d) {
  Variable sorted_data =
      makeVariable<double>(Dims{Dim::Event}, Shape{3}, Values{4, 1, 2},
                           Variances{4, 1, 3}, units::m);
  Variable sorted_x = this->template make_coord<0>(Dim::Event, 1, 3, 2);
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{3},
                                            Values{false, true, false});
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}}, {{"mask", sorted_mask}});

  const auto binned = bin(this->table, {this->edges_x});

  EXPECT_EQ(binned.dims(), Dimensions(Dim::X, 2));
  EXPECT_EQ(binned.coords()[Dim::X], this->edges_x);
  EXPECT_EQ(binned.coords()[Dim("scalar")], this->scalar);
  EXPECT_EQ(binned.template values<bucket<DataArray>>()[0],
            sorted_table.slice({Dim::Event, 0, 1}));
  EXPECT_EQ(binned.template values<bucket<DataArray>>()[1],
            sorted_table.slice({Dim::Event, 1, 3}));
}

TYPED_TEST(DataArrayBinTest, 2d) {
  this->table.coords().set(Dim::Y, this->y);
  Variable sorted_data =
      makeVariable<double>(Dims{Dim::Event}, Shape{3}, Values{4, 1, 2},
                           Variances{4, 1, 3}, units::m);
  Variable sorted_x = this->template make_coord<0>(Dim::Event, 1, 3, 2);
  Variable sorted_y = this->template make_coord<1>(Dim::Event, 2, 1, 2);
  Variable sorted_mask = makeVariable<bool>(Dims{Dim::Event}, Shape{3},
                                            Values{false, true, false});
  DataArray sorted_table =
      DataArray(sorted_data, {{Dim::X, sorted_x}, {Dim::Y, sorted_y}},
                {{"mask", sorted_mask}});

  const auto binned = bin(this->table, {this->edges_x, this->edges_y});

  EXPECT_EQ(binned.dims(), Dimensions({Dim::X, Dim::Y}, {2, 2}));
  EXPECT_EQ(binned.coords()[Dim::X], this->edges_x);
  EXPECT_EQ(binned.coords()[Dim::Y], this->edges_y);
  EXPECT_EQ(binned.coords()[Dim("scalar")], this->scalar);
  const auto empty_bin = sorted_table.slice({Dim::Event, 0, 0});
  EXPECT_EQ(binned.template values<bucket<DataArray>>()[0], empty_bin);
  EXPECT_EQ(binned.template values<bucket<DataArray>>()[1],
            sorted_table.slice({Dim::Event, 0, 1}));
  EXPECT_EQ(binned.template values<bucket<DataArray>>()[2], empty_bin);
  EXPECT_EQ(binned.template values<bucket<DataArray>>()[3],
            sorted_table.slice({Dim::Event, 1, 3}));

  EXPECT_EQ(bin(bin(this->table, {this->edges_x}), {this->edges_y}), binned);
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
  auto expected = copy(table);
  expected.coords().erase(Dim("label"));
  EXPECT_EQ(binned.dims(), groups.dims());
  EXPECT_EQ(binned.values<core::bin<DataArray>>()[1],
            expected.slice({Dim::Row, 2, 3}));
  EXPECT_EQ(binned.values<core::bin<DataArray>>()[0].slice({Dim::Row, 0}),
            expected.slice({Dim::Row, 0}));
  EXPECT_EQ(binned.values<core::bin<DataArray>>()[0].slice({Dim::Row, 1}),
            expected.slice({Dim::Row, 4}));
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

  void expect_near(const DataArray &a, const DataArray &b, double rtol = 1e-14,
                   double atol = 0.0) {
    const auto tolerance =
        values(max(bins_sum(a.data())) * (rtol * units::one));
    EXPECT_TRUE(
        all(isclose(values(bins_sum(a.data())), values(bins_sum(b.data())),
                    atol * units::one, tolerance))
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
  EXPECT_THROW(bin(table, {}), std::invalid_argument);
}

TEST_P(BinTest, edges_too_short) {
  const auto table = GetParam();
  const auto edges = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1});
  EXPECT_THROW(bin(table, {edges}), except::BinEdgeError);
}

TEST_P(BinTest, rebin_no_event_coord) {
  const auto table = GetParam();
  const auto x = bin(table, {edges_x_coarse});
  bins_view<DataArray>(x.data()).coords().erase(Dim::X);
  EXPECT_THROW_DISCARD(bin(x, {edges_x}), except::BinEdgeError);
}

TEST_P(BinTest, bin_using_attr) {
  auto table = GetParam();
  const auto da_coord = bin(table, {edges_x});
  table.attrs().set(Dim::X, table.coords().extract(Dim::X));
  const auto da_attr = bin(table, {edges_x});
  auto da_attr_bins_view = bins_view<DataArray>(da_attr.data());
  da_attr_bins_view.coords().set(Dim::X,
                                 da_attr_bins_view.attrs().extract(Dim::X));
  EXPECT_EQ(da_coord, da_attr);
}

TEST_P(BinTest, rebin_using_attr) {
  auto table = GetParam();
  const auto da_coord = bin(bin(table, {edges_x}), {edges_x_coarse});
  table.attrs().set(Dim::X, table.coords().extract(Dim::X));
  const auto da_attr = bin(bin(table, {edges_x}), {edges_x_coarse});
  auto da_attr_bins_view = bins_view<DataArray>(da_attr.data());
  da_attr_bins_view.coords().set(Dim::X,
                                 da_attr_bins_view.attrs().extract(Dim::X));
  EXPECT_EQ(da_coord, da_attr);
}

TEST_P(BinTest, rebin_using_attr_in_new_dimension) {
  const auto z_coord = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                            Values{-10., -5.0, 0.5, 7.5});
  const auto edges_z_coarse =
      makeVariable<double>(Dims{Dim::Z}, Shape{3}, Values{-11., 0.0, 8.0});
  const auto table = GetParam();
  auto da_coord = bin(table, {edges_x});
  auto da_attr = bin(table, {edges_x});
  da_coord.coords().set(Dim::Z, z_coord);
  da_attr.attrs().set(Dim::Z, z_coord);
  const auto out_coord = bin(da_coord, {edges_z_coarse});
  const auto out_attr = bin(da_attr, {edges_z_coarse});
  EXPECT_EQ(out_coord, out_attr);
}

TEST_P(BinTest, rebin_existing_binning_attr_and_event_coord) {
  auto table = GetParam();
  const auto da_coord = bin(bin(table, {edges_x}), {edges_x_coarse});
  auto da_attr_temp = bin(table, {edges_x});
  da_attr_temp.attrs().set(Dim::X, da_attr_temp.coords().extract(Dim::X));
  const auto da_attr = bin(da_attr_temp, {edges_x_coarse});
  EXPECT_EQ(da_coord, da_attr);
}

TEST_P(BinTest, rebin_existing_binning_attr_and_event_attr) {
  auto table = GetParam();
  const auto da_coord = bin(bin(table, {edges_x}), {edges_x_coarse});
  table.attrs().set(Dim::X, table.coords().extract(Dim::X));
  auto da_attr_temp = bin(table, {edges_x});
  da_attr_temp.attrs().set(Dim::X, da_attr_temp.coords().extract(Dim::X));
  const auto da_attr = bin(da_attr_temp, {edges_x_coarse});
  auto da_attr_bins_view = bins_view<DataArray>(da_attr.data());
  da_attr_bins_view.coords().set(Dim::X,
                                 da_attr_bins_view.attrs().extract(Dim::X));
  EXPECT_EQ(da_coord, da_attr);
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

TEST_P(
    BinTest,
    rebin_inner_1d_coord_to_2d_coord_gives_same_result_as_direct_binning_to_2d_coord) {
  auto table = GetParam();
  auto xy = bin(table, {edges_x_coarse, edges_y_coarse});
  Variable edges_y_2d = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                             Values{-2, 1, 2, -3, 0, 3});
  // With bin-coord for Y
  expect_near(bin(xy, {edges_y_2d}),
              bin(bin(table, {edges_x_coarse}), {edges_y_2d}));
  // Without bin-coord for Y
  xy.coords().erase(Dim::Y);
  expect_near(bin(xy, {edges_y_2d}),
              bin(bin(table, {edges_x_coarse}), {edges_y_2d}));
}

TEST_P(BinTest, rebin_2d_with_2d_coord) {
  auto table = GetParam();
  auto xy = bin(table, {edges_x_coarse, edges_y_coarse});
  Variable edges_y_2d = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                             Values{-2, 1, 2, -3, 0, 3});
  xy.coords().set(Dim::Y, edges_y_2d);
  bins_view<DataArray>(xy.data()).coords()[Dim::Y] += 0.5 * units::one;
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
  // Unchanged outer binning
  EXPECT_EQ(bin(xy, {edges_x_coarse, edges_y_coarse}),
            bin(xy, {edges_y_coarse}));
}

TEST_P(BinTest, rebin_2d_with_2d_attr) {
  auto table = GetParam();
  table.attrs().set(Dim::Y, table.coords().extract(Dim::Y));
  auto xy = bin(table, {edges_x_coarse, edges_y_coarse});
  Variable edges_y_2d = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                                             Values{-2, 1, 2, -3, 0, 3});
  xy.coords().erase(Dim::Y);
  xy.attrs().set(Dim::Y, edges_y_2d);
  bins_view<DataArray>(xy.data()).attrs()[Dim::Y] += 0.5 * units::one;
  EXPECT_THROW(bin(xy, {edges_x_coarse}), except::DimensionError);
  EXPECT_NO_THROW(bin(xy, {edges_x_coarse, edges_y_coarse}));
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

TEST_P(BinTest, rebin_coarse_to_fine_2d_outer_no_coord) {
  const auto table = GetParam();
  auto xy_coarse = bin(table, {edges_x_coarse, edges_y});
  auto xy = bin(table, {edges_x, edges_y});
  xy_coarse.coords().erase(Dim::X);
  expect_near(bin(xy_coarse, {edges_x}), xy);
}

TEST_P(BinTest, rebin_empty_dim) {
  const auto table = GetParam();
  const auto xy = bin(table, {edges_x, edges_y});
  const auto sx = Slice{Dim::X, 0, 0};
  const auto sy = Slice{Dim::Y, 0, 0};
  EXPECT_EQ(bin(xy.slice(sx), {edges_y_coarse}),
            bin(xy, {edges_y_coarse}).slice(sx));
  EXPECT_EQ(bin(xy.slice(sy), {edges_x_coarse}),
            bin(xy, {edges_x_coarse}).slice(sy));
  EXPECT_EQ(bin(xy.slice(sx).slice(sy), {edges_x_coarse}),
            bin(xy, {edges_x_coarse}).slice(sy));
  EXPECT_EQ(bin(xy.slice(sx).slice(sy), {edges_y_coarse}),
            bin(xy, {edges_y_coarse}).slice(sx));
}

TEST_P(BinTest, group_and_bin) {
  const auto table = GetParam();
  const auto x_group = bin(table, {edges_x}, {groups});
  const auto group = bin(table, {}, {groups});
  EXPECT_EQ(bin(group, {edges_x}, {}), x_group);
}

TEST_P(BinTest, rebin_masked) {
  auto table = GetParam();
  table.setUnit(units::counts); // we want to use `histogram` for comparison
  auto binned = bin(table, {edges_x_coarse});
  binned.masks().set("x-mask", makeVariable<bool>(Dims{Dim::X}, Shape{2},
                                                  Values{false, true}));
  EXPECT_EQ(bins_sum(bin(binned, {edges_x})), histogram(binned, edges_x));
  if (table.dims().volume() > 0) {
    EXPECT_NE(bin(binned, {edges_x}), bin(table, {edges_x}));
    EXPECT_NE(bins_sum(bin(binned, {edges_x})), histogram(table, edges_x));
    binned.masks().erase("x-mask");
    EXPECT_EQ(bin(binned, {edges_x}), bin(table, {edges_x}));
    EXPECT_EQ(bins_sum(bin(binned, {edges_x})), histogram(table, edges_x));
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
  // Using bin coord (instead of event coord) for binning.
  // Edges giving same grouping as existing => data matches
  auto edges = makeVariable<double>(Dims{Dim("group")}, Shape{6},
                                    Values{-2, -1, 0, 1, 2, 3});
  EXPECT_EQ(bin(binned, {edges}).data(), binned.data());

  // Fewer bins than groups
  edges = makeVariable<double>(Dims{Dim("group")}, Shape{3}, Values{-2, 0, 2});
  EXPECT_NO_THROW(bin(binned, {edges}));
}

TEST_P(BinTest, rebin_various_edges_1d) {
  // Trying to cover potential edge case in the bin sizes setup logic. No assert
  // since in general it is hard to come up with the expected result.
  using units::one;
  std::vector<Variable> edges;
  edges.emplace_back(linspace(-2.0 * one, 1.2 * one, Dim::X, 4));
  edges.emplace_back(linspace(-2.0 * one, 1.2 * one, Dim::X, 72));
  edges.emplace_back(linspace(-1.23 * one, 1.2 * one, Dim::X, 45));
  edges.emplace_back(linspace(-1.23 * one, 1.1 * one, Dim::X, 45));
  edges.emplace_back(linspace(1.1 * one, 1.2 * one, Dim::X, 128));
  edges.emplace_back(linspace(1.1 * one, 1.101 * one, Dim::X, 128));
  const auto table = GetParam();
  for (const auto &e0 : edges)
    for (const auto &e1 : edges) {
      auto binned = bin(table, {e0});
      const auto x = binned.coords()[Dim::X];
      const auto len = x.dims()[Dim::X];
      binned.coords().set(Dim::X, (0.5 * one) * (x.slice({Dim::X, 0, len - 1}) +
                                                 x.slice({Dim::X, 1, len})));
      bin(binned, {e1});
    }
  for (const auto &e0 : edges)
    for (const auto &e1 : edges)
      bin(bin(table, {e0}), {e1});
}

TEST_P(BinTest, erase_binning_and_bin_along_different_dimension) {
  const auto table = GetParam();
  const auto binned_along_x = bin(table, {edges_x});
  const auto binned_along_y = bin(table, {edges_y});

  const std::vector<Dim> clear_binning_from_dimension = {Dim::X};
  const auto test_output =
      bin(binned_along_x, {edges_y}, {}, clear_binning_from_dimension);
  // Expect result of clearing x binning and adding y binning to be the same
  // as binning the original data along y
  expect_near(test_output, binned_along_y);
}

TEST_P(BinTest, erase_binning_and_rebin_trivial_along_different_dimension) {
  const auto table = GetParam();
  const auto binned_along_x = bin(table, {edges_x, edges_y});
  const auto binned_along_y = bin(table, {edges_y});

  const std::vector<Dim> clear_binning_from_dimension = {Dim::X};
  const auto test_output =
      bin(binned_along_x, {edges_y}, {}, clear_binning_from_dimension);
  // Expect result of clearing x binning and adding y binning to be the same
  // as binning the original data along y
  expect_near(test_output, binned_along_y);
}

TEST_P(BinTest,
       erase_binning_and_rebin_coarse_to_fine_along_different_dimension) {
  const auto table = GetParam();
  const auto binned_along_x = bin(table, {edges_x, edges_y_coarse});
  const auto binned_along_y = bin(table, {edges_y});

  const std::vector<Dim> clear_binning_from_dimension = {Dim::X};
  const auto test_output =
      bin(binned_along_x, {edges_y}, {}, clear_binning_from_dimension);
  // Expect result of clearing x binning and adding y binning to be the same
  // as binning the original data along y
  expect_near(test_output, binned_along_y);
}

TEST_P(BinTest,
       erase_binning_and_rebin_fine_to_coarse_along_different_dimension) {
  const auto table = GetParam();
  const auto binned_along_x = bin(table, {edges_x, edges_y});
  const auto binned_along_y = bin(table, {edges_y_coarse});

  const std::vector<Dim> clear_binning_from_dimension = {Dim::X};
  const auto test_output =
      bin(binned_along_x, {edges_y_coarse}, {}, clear_binning_from_dimension);
  // Expect result of clearing x binning and adding y binning to be the same
  // as binning the original data along y
  expect_near(test_output, binned_along_y, 1e-14, 1e-13);
}

TEST_P(BinTest, error_if_erase_binning_and_try_rebin_along_same_dimension) {
  const auto table = GetParam();
  const auto binned_along_x = bin(table, {edges_x_coarse});

  // Trying to rebin in X but also clear binning from X
  const std::vector<Dim> erase_binning_from_dimension = {Dim::X};
  EXPECT_THROW(bin(binned_along_x, {edges_x}, {}, erase_binning_from_dimension),
               except::DimensionError);
}

TEST(BinTest, twod_not_supported) {
  const Dimensions dims({{Dim::X, 2}, {Dim::Y, 2}});
  const auto data = makeVariable<double>(dims, Values{0, 1, 2, 3});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0.1, 0.2});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1.1, 1.2});
  const DataArray table(data, {{Dim::X, x}, {Dim::Y, y}});

  const auto edges_x =
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{0, 2});
  const auto edges_y =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 2});

  EXPECT_THROW(bin(table, {edges_x}), except::BinnedDataError);
  EXPECT_THROW(bin(table, {edges_y}), except::BinnedDataError);
}

TEST_P(BinTest, new_dim_existing_coord) {
  const auto table = GetParam();
  auto da = bin(table, {edges_x});
  const auto expected = bin(da, {edges_y});
  // This case arises, e.g., after transform_coords when the input dimension is
  // not renamed. Ensure we do not enter the code branch handling existing
  // binning since this would throw.
  auto edges = copy(edges_y);
  edges.rename(Dim::Y, Dim::X);
  da.coords().set(Dim::Y, edges);
  EXPECT_EQ(bin(da, {edges_y}), expected);
}
