// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <limits>
#include <vector>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/mean.h"
#include "scipp/dataset/sum.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/special_values.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(SumTest, masked_data_array) {
  const auto var =
      makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
                           Values{1.0, 2.0, 3.0, 4.0});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);
  const auto sumX = makeVariable<double>(Dimensions{Dim::Y, 2}, sc_units::m,
                                         Values{1.0, 3.0});
  const auto sumY = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                         Values{4.0, 6.0});
  EXPECT_EQ(sum(a, Dim::X).data(), sumX);
  EXPECT_EQ(sum(a, Dim::Y).data(), sumY);
  EXPECT_FALSE(sum(a, Dim::X).masks().contains("mask"));
  auto summedY = sum(a, Dim::Y);
  EXPECT_TRUE(summedY.masks().contains("mask"));
  // Ensure reduction operation does NOT share the unrelated mask
  EXPECT_EQ(summedY.masks()["mask"], mask);
  summedY.masks()["mask"] &= ~mask;
  EXPECT_NE(summedY.masks()["mask"], mask);
}

TEST(SumTest, masked_data_with_special_vals) {
  const auto var = makeVariable<double>(
      Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
      Values{1.0, std::numeric_limits<double>::quiet_NaN(), 3.0, 4.0});
  const auto mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                       Values{false, true, false, false});

  DataArray a(var);
  a.masks().set("mask", mask);

  const auto sumX = makeVariable<double>(Dimensions{Dim::Y, 2}, sc_units::m,
                                         Values{1.0, 7.0});
  const auto sumY = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                         Values{4.0, 4.0});

  EXPECT_EQ(sum(a, Dim::X).data(), sumX);
  EXPECT_EQ(sum(a, Dim::Y).data(), sumY);
}

TEST(SumTest, masked_data_array_two_masks) {
  const auto var =
      makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
                           Values{1.0, 2.0, 3.0, 4.0});
  const auto maskX =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  const auto maskY =
      makeVariable<bool>(Dimensions{Dim::Y, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("x", maskX);
  a.masks().set("y", maskY);
  const auto sumX = makeVariable<double>(Dimensions{Dim::Y, 2}, sc_units::m,
                                         Values{1.0, 3.0});
  const auto sumY = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                         Values{1.0, 2.0});
  EXPECT_EQ(sum(a, Dim::X).data(), sumX);
  EXPECT_EQ(sum(a, Dim::Y).data(), sumY);
  EXPECT_FALSE(sum(a, Dim::X).masks().contains("x"));
  EXPECT_TRUE(sum(a, Dim::X).masks().contains("y"));
  EXPECT_TRUE(sum(a, Dim::Y).masks().contains("x"));
  EXPECT_FALSE(sum(a, Dim::Y).masks().contains("y"));
}

TEST(SumTest, mask_dim_order_does_not_overrule_data_dim_order) {
  Variable var =
      makeVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z}, Shape{1, 1, 1});
  DataArray da(var);
  da.masks().set("mask", makeVariable<bool>(Dims{Dim::Y, Dim::X, Dim::Z},
                                            Shape{1, 1, 1}, Values{true}));
  EXPECT_EQ(sum(da, Dim::Z).dims(), da.slice({Dim::Z, 0}).dims());
}

TEST(SumTest, mask_order_does_not_overrule_data_dim_order) {
  Variable var =
      makeVariable<double>(Dims{Dim::X, Dim::Y, Dim::Z}, Shape{1, 1, 1});
  DataArray da(var);
  // Would pass with just one mask
  da.masks().set("yz", makeVariable<bool>(Dims{Dim::Y, Dim::Z}, Shape{1, 1},
                                          Values{true}));
  // Mask dict keeps insertion order, masks merged in order "Y then X"
  da.masks().set("xy", makeVariable<bool>(Dims{Dim::X, Dim::Z}, Shape{1, 1},
                                          Values{true}));
  EXPECT_EQ(sum(da, Dim::Z).dims(), da.slice({Dim::Z, 0}).dims());
}

class Sum2dCoordTest : public ::testing::Test {
protected:
  Variable var{makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    Values{1.0, 2.0, 3.0, 4.0})};
};

TEST_F(Sum2dCoordTest, data_array_2d_coord) {
  DataArray a(var, {{Dim::X, var}});
  // Coord is for summed dimension -> drop.
  EXPECT_FALSE(sum(a, Dim::X).coords().contains(Dim::X));
}

TEST_F(Sum2dCoordTest, data_array_2d_labels_dropped) {
  DataArray a(var, {{Dim("xlabels"), var}});
  // Coord depend on summed dimension -> drop.
  EXPECT_FALSE(sum(a, Dim::X).coords().contains(Dim("xlabels")));
  EXPECT_FALSE(sum(a, Dim::Y).coords().contains(Dim("xlabels")));
  // Drops even dimension coords
  EXPECT_FALSE(sum(a, Dim::Y).coords().contains(Dim::X));
}

TEST_F(Sum2dCoordTest, data_array_independent_2d_labels_preserved) {
  Variable var3d{makeVariable<double>(Dims{Dim::Z, Dim::Y, Dim::X},
                                      Shape{1, 2, 2},
                                      Values{1.0, 2.0, 3.0, 4.0})};
  DataArray a(var3d, {{Dim::X, var}});
  EXPECT_FALSE(sum(a, Dim::X).coords().contains(Dim::X));
  EXPECT_FALSE(sum(a, Dim::Y).coords().contains(Dim::X));
  EXPECT_TRUE(sum(a, Dim::Z).coords().contains(Dim::X));
}

class ReduceBinnedTest : public ::testing::Test {
protected:
  Variable indices = makeVariable<index_pair>(
      Dims{Dim::Y}, Shape{3},
      Values{std::pair{0, 2}, std::pair{2, 2}, std::pair{2, 5}});
  Variable data = makeVariable<double>(Dims{Dim::Event}, Shape{5}, sc_units::m,
                                       Values{1, 2, 3, 4, 5});
  DataArray buffer = DataArray(data);
  Variable binned = make_bins(indices, Dim::Event, buffer);
};

TEST_F(ReduceBinnedTest, sum) {
  EXPECT_EQ(sum(binned), makeVariable<double>(Dims{}, sc_units::m, Values{15}));
}

TEST_F(ReduceBinnedTest, sum_masked_events) {
  buffer.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::Event}, Shape{5},
                                 Values{true, true, false, true, false}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(sum(binned),
            makeVariable<double>(Dims{}, sc_units::m, Values{3 + 5}));
}

TEST_F(ReduceBinnedTest, sum_masked_events_bool) {
  buffer.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::Event}, Shape{5},
                                 Values{true, true, false, true, false}));
  buffer.setData(makeVariable<bool>(Dims{Dim::Event}, Shape{5},
                                    Values{false, true, true, false, true}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(sum(binned),
            makeVariable<int64_t>(Dims{}, sc_units::none, Values{2}));
}

TEST_F(ReduceBinnedTest, mean) {
  EXPECT_EQ(mean(binned),
            makeVariable<double>(Dims{}, sc_units::m, Values{(15.0) / 5}));
}

TEST_F(ReduceBinnedTest, mean_masked_events) {
  buffer.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::Event}, Shape{5},
                                 Values{true, true, false, true, false}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(mean(binned),
            makeVariable<double>(Dims{}, sc_units::m, Values{(3.0 + 5.0) / 2}));
}

class ReduceMaskedBinnedTest : public ::testing::Test {
protected:
  Variable indices = makeVariable<index_pair>(
      Dims{Dim::Y}, Shape{3},
      Values{std::pair{0, 2}, std::pair{2, 2}, std::pair{2, 5}});
  Variable data = makeVariable<double>(Dims{Dim::Event}, Shape{5}, sc_units::m,
                                       Values{1, 2, 3, 4, 5});
  DataArray buffer = DataArray(data);
  Variable binned_var = make_bins(indices, Dim::Event, buffer);
  DataArray binned =
      DataArray(binned_var, {},
                {{"mask", makeVariable<bool>(Dims{Dim::Y}, Shape{3},
                                             Values{true, false, false})}});
};

TEST_F(ReduceMaskedBinnedTest, sum) {
  EXPECT_EQ(sum(binned),
            DataArray(makeVariable<double>(Dims{}, sc_units::m, Values{12})));
}

TEST_F(ReduceMaskedBinnedTest, sum_masked_events) {
  buffer.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::Event}, Shape{5},
                                 Values{true, false, false, false, true}));
  binned = DataArray(make_bins(indices, Dim::Event, buffer), {},
                     {{"mask", binned.masks()["mask"]}});
  EXPECT_EQ(sum(binned),
            DataArray(makeVariable<double>(Dims{}, sc_units::m, Values{7})));
}

TEST_F(ReduceMaskedBinnedTest, mean) {
  EXPECT_EQ(mean(binned),
            DataArray(makeVariable<double>(Dims{}, sc_units::m, Values{4})));
}

TEST_F(ReduceMaskedBinnedTest, mean_masked_events) {
  buffer.masks().set(
      "mask", makeVariable<bool>(Dims{Dim::Event}, Shape{5},
                                 Values{true, false, false, false, true}));
  binned = DataArray(make_bins(indices, Dim::Event, buffer), {},
                     {{"mask", binned.masks()["mask"]}});
  EXPECT_EQ(mean(binned),
            DataArray(makeVariable<double>(Dims{}, sc_units::m, Values{3.5})));
}

class Reduce2dBinnedTest : public ::testing::Test {
protected:
  Variable indices =
      makeVariable<index_pair>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                               Values{std::pair{0, 2}, std::pair{2, 2},
                                      std::pair{2, 5}, std::pair{5, 7}});
  Variable data = makeVariable<double>(Dims{Dim::Event}, Shape{7}, sc_units::m,
                                       Values{1, 2, 3, 4, 5, 6, 7});
  DataArray buffer = DataArray(data);
  Variable binned = make_bins(indices, Dim::Event, buffer);
};

TEST_F(Reduce2dBinnedTest, sum_all_dims) {
  EXPECT_EQ(sum(binned), makeVariable<double>(Dims{}, sc_units::m, Values{28}));
}

TEST_F(Reduce2dBinnedTest, sum_inner_dim) {
  EXPECT_EQ(
      sum(binned, Dim::X),
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m, Values{3, 25}));
}

TEST_F(Reduce2dBinnedTest, sum_outer_dim) {
  EXPECT_EQ(sum(binned, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                 Values{15, 13}));
}

TEST_F(Reduce2dBinnedTest, sum_masked_all_dims) {
  buffer.masks().set(
      "mask",
      makeVariable<bool>(Dims{Dim::Event}, Shape{7},
                         Values{true, false, false, false, true, true, true}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(sum(binned), makeVariable<double>(Dims{}, sc_units::m, Values{9}));
}

TEST_F(Reduce2dBinnedTest, sum_masked_inner_dims) {
  buffer.masks().set(
      "mask",
      makeVariable<bool>(Dims{Dim::Event}, Shape{7},
                         Values{true, false, false, false, true, true, true}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(
      sum(binned, Dim::X),
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m, Values{2, 7}));
}

TEST_F(Reduce2dBinnedTest, sum_masked_outer_dims) {
  buffer.masks().set(
      "mask",
      makeVariable<bool>(Dims{Dim::Event}, Shape{7},
                         Values{true, false, false, false, true, true, true}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(
      sum(binned, Dim::Y),
      makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m, Values{9, 0}));
}

TEST_F(Reduce2dBinnedTest, mean_all_dims) {
  EXPECT_EQ(mean(binned),
            makeVariable<double>(Dims{}, sc_units::m, Values{28.0 / 7}));
}

TEST_F(Reduce2dBinnedTest, mean_inner_dim) {
  EXPECT_EQ(mean(binned, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m,
                                 Values{3.0 / 2, 25.0 / 5}));
}

TEST_F(Reduce2dBinnedTest, mean_outer_dim) {
  EXPECT_EQ(mean(binned, Dim::Y),
            makeVariable<double>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                 Values{15.0 / 5, 13.0 / 2}));
}

TEST_F(Reduce2dBinnedTest, mean_masked_all_dims) {
  buffer.masks().set(
      "mask",
      makeVariable<bool>(Dims{Dim::Event}, Shape{7},
                         Values{true, false, false, false, true, true, true}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(mean(binned),
            makeVariable<double>(Dims{}, sc_units::m, Values{9.0 / 3}));
}

TEST_F(Reduce2dBinnedTest, mean_masked_inner_dims) {
  buffer.masks().set(
      "mask",
      makeVariable<bool>(Dims{Dim::Event}, Shape{7},
                         Values{true, false, false, false, true, true, true}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(mean(binned, Dim::X),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, sc_units::m,
                                 Values{2.0 / 1, 7.0 / 2}));
}

TEST_F(Reduce2dBinnedTest, mean_masked_outer_dims) {
  buffer.masks().set(
      "mask",
      makeVariable<bool>(Dims{Dim::Event}, Shape{7},
                         Values{true, false, false, false, true, true, true}));
  binned = make_bins(indices, Dim::Event, buffer);
  EXPECT_EQ(mean(binned, Dim::Y).slice({Dim::X, 0}).value<double>(), 9.0 / 3);
  EXPECT_TRUE(isnan(mean(binned, Dim::Y).slice({Dim::X, 1})).value<bool>());
}
