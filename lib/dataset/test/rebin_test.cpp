// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/dataset/rebin.h"
#include "scipp/dataset/shape.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/rebin.h"
#include "scipp/variable/shape.h"

using namespace scipp;
using namespace scipp::dataset;

class RebinTest : public ::testing::Test {
protected:
  Variable counts =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 4}, sc_units::counts,
                           Values{1, 2, 3, 4, 5, 6, 7, 8});
  Variable x =
      makeVariable<double>(Dims{Dim::X}, Shape{5}, Values{1, 2, 3, 4, 5});
  Variable y = makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3});
  DataArray array{counts, {{Dim::X, x}, {Dim::Y, y}}, {}};
  DataArray array_with_variances{
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 4}, sc_units::counts,
                           Values{1, 2, 3, 4, 5, 6, 7, 8},
                           Variances{9, 10, 11, 12, 13, 14, 15, 16}),
      {{Dim::X, x}, {Dim::Y, y}},
      {}};
};

TEST_F(RebinTest, inner_stride1_data_array) {
  auto edges = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 3, 5});
  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                          sc_units::counts,
                                          Values{3, 7, 11, 15}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {});
  ASSERT_EQ(rebin(array, Dim::X, edges), expected);
}

TEST_F(RebinTest, inner_stride1_strided_edges) {
  auto buffer = makeVariable<double>(Dims{Dim::X, Dim::Z}, Shape{3, 2},
                                     Values{1, 0, 3, 0, 5, 0});
  auto edges = buffer.slice({Dim::Z, 0});
  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                          sc_units::counts,
                                          Values{3, 7, 11, 15}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {});
  ASSERT_EQ(rebin(array, Dim::X, edges), expected);
}

TEST_F(RebinTest, outer_stride1_data_array) {
  auto edges = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 3, 5});
  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                          sc_units::counts,
                                          Values{3, 7, 11, 15}),
                     {{Dim::X, edges}, {Dim::Y, y}}, {});
  ASSERT_EQ(rebin(transpose(array), Dim::X, edges), transpose(expected));
}

TEST_F(RebinTest, inner_data_array_with_variances) {
  auto edges = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 3, 5});
  DataArray expected(
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::counts,
                           Values{3, 7, 11, 15}, Variances{19, 23, 27, 31}),
      {{Dim::X, edges}, {Dim::Y, y}}, {});

  ASSERT_EQ(rebin(array_with_variances, Dim::X, edges), expected);
}

TEST_F(RebinTest, inner_data_array_unaligned_edges) {
  auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.5, 3.5, 5.5});
  DataArray expected(
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::counts,
                           Values{0.5 * 1 + 2 + 0.5 * 3, 0.5 * 3 + 4,
                                  0.5 * 5 + 6 + 0.5 * 7, 0.5 * 7 + 8}),
      {{Dim::X, edges}, {Dim::Y, y}}, {});

  ASSERT_EQ(rebin(array, Dim::X, edges), expected);
}

TEST_F(RebinTest, outer_data_array) {
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 3});
  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 4},
                                          sc_units::counts,
                                          Values{6, 8, 10, 12}),
                     {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array, Dim::Y, edges), expected);
}

TEST_F(RebinTest, outer_data_array_different_edge_dtype) {
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 3});
  DataArray expected(makeVariable<float>(Dims{Dim::Y, Dim::X}, Shape{1, 4},
                                         sc_units::counts,
                                         Values{6, 8, 10, 12}),
                     {{Dim::X, x}, {Dim::Y, edges}}, {});
  DataArray array_float(astype(counts, dtype<float>),
                        {{Dim::X, x}, {Dim::Y, y}});

  ASSERT_EQ(rebin(array_float, Dim::Y, edges), expected);
}

TEST_F(RebinTest, outer_data_array_with_variances) {
  auto edges = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 3});
  DataArray expected(
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{1, 4}, sc_units::counts,
                           Values{6, 8, 10, 12}, Variances{22, 24, 26, 28}),
      {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array_with_variances, Dim::Y, edges), expected);
}

TEST_F(RebinTest, outer_data_array_unaligned_edges) {
  auto edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1.0, 2.5, 3.5});
  DataArray expected(
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 4}, sc_units::counts,
                           Values{1.0 + 0.5 * 5, 2.0 + 0.5 * 6, 3.0 + 0.5 * 7,
                                  4.0 + 0.5 * 8, 0.5 * 5, 0.5 * 6, 0.5 * 7,
                                  0.5 * 8}),
      {{Dim::X, x}, {Dim::Y, edges}}, {});

  ASSERT_EQ(rebin(array, Dim::Y, edges), expected);
}

TEST_F(RebinTest, keeps_unrelated_labels_but_drops_others) {
  const auto labels_x = makeVariable<double>(Dims{Dim::X}, Shape{4});
  const auto labels_y = makeVariable<double>(Dims{Dim::Y}, Shape{2});
  const DataArray a(counts, {{Dim::X, x},
                             {Dim::Y, y},
                             {Dim("labels_x"), labels_x},
                             {Dim("labels_y"), labels_y}});
  const auto edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 3, 5});
  DataArray expected(
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::counts,
                           Values{3, 7, 11, 15}),
      {{Dim::X, edges}, {Dim::Y, y}, {Dim("labels_y"), labels_y}});

  ASSERT_EQ(rebin(a, Dim::X, edges), expected);
}

TEST_F(RebinTest, rebin_with_ragged_coord) {
  Variable data = makeVariable<double>(
      Dims{Dim::Z, Dim::Y, Dim::X}, Shape{3, 2, 4}, sc_units::counts,
      Values{1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
             13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24});
  Variable var_x = makeVariable<double>(
      Dims{Dim::Y, Dim::X}, Shape{2, 5},
      Values{1.0, 2.0, 3.0, 4.0, 5.0, 1.1, 2.2, 3.3, 4.4, 5.5});
  Variable var_y =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3});
  Variable var_z =
      makeVariable<double>(Dims{Dim::Z}, Shape{4}, Values{1, 2, 3, 4});

  DataArray da{data, {{Dim::X, var_x}, {Dim::Y, var_y}, {Dim::Z, var_z}}, {}};

  auto edges =
      makeVariable<double>(Dims{Dim::Z}, Shape{3}, Values{0.0, 2.5, 5.0});
  DataArray expected(
      makeVariable<double>(
          Dims{Dim::Z, Dim::Y, Dim::X}, Shape{2, 2, 4}, sc_units::counts,
          Values{5.5, 7.0, 8.5, 10.0, 11.5, 13.0, 14.5, 16.0, 21.5, 23.0, 24.5,
                 26.0, 27.5, 29.0, 30.5, 32.0}),
      {{Dim::Z, edges}, {Dim::X, var_x}, {Dim::Y, var_y}}, {});

  ASSERT_EQ(rebin(da, Dim::Z, edges), expected);
}

class RebinRaggedTest : public ::testing::Test {
protected:
  Variable counts =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 3}, sc_units::counts,
                           Values{1, 2, 3, 4, 5, 6});
  Variable x = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 4},
                                    Values{1, 2, 3, 4, 5, 6, 7, 8});
  DataArray da{counts, {{Dim::X, x}}};
  Variable edges =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{0, 3, 7});
};

TEST_F(RebinRaggedTest, stride1_data_and_edges) {
  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                          sc_units::counts, Values{3, 3, 0, 9}),
                     {{Dim::X, edges}});
  ASSERT_EQ(rebin(da, Dim::X, edges), expected);
  ASSERT_EQ(rebin(transpose(da), Dim::X, edges), transpose(expected));
}

TEST_F(RebinRaggedTest, stride1_edges) {
  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                          sc_units::counts, Values{3, 3, 0, 9}),
                     {{Dim::X, edges}});
  ASSERT_EQ(rebin(copy(transpose(da)), Dim::X, edges), transpose(expected));
}

TEST_F(RebinRaggedTest, stride1_data) {
  da.coords().set(Dim::X, copy(transpose(x)));
  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                          sc_units::counts, Values{3, 3, 0, 9}),
                     {{Dim::X, edges}});
  ASSERT_EQ(rebin(da, Dim::X, edges), expected);
}

TEST_F(RebinRaggedTest, no_stride1) {
  da.coords().set(Dim::X, copy(transpose(x)));
  DataArray expected(makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                          sc_units::counts, Values{3, 3, 0, 9}),
                     {{Dim::X, edges}});
  ASSERT_EQ(rebin(copy(transpose(da)), Dim::X, edges), transpose(expected));
}

TEST(RebinWithMaskTest, applies_mask_along_rebin_dim) {
  DataArray da(
      broadcast(makeVariable<double>(Dimensions{Dim::X, 5}, sc_units::counts,
                                     Values{1, 2, 3, 4, 5}),
                Dimensions({Dim::Y, Dim::X}, {5, 5})),
      {{Dim::X, makeVariable<double>(Dimensions{Dim::X, 6},
                                     Values{1, 2, 3, 4, 5, 6})}});

  da.masks().set("mask_x",
                 makeVariable<bool>(Dimensions{Dim::X, 5},
                                    Values{false, false, true, false, false}));

  const auto edges =
      makeVariable<double>(Dimensions{Dim::X, 3}, Values{1, 3, 5});
  const auto result = rebin(da, Dim::X, edges);

  EXPECT_FALSE(result.masks().contains("mask_x"));
  EXPECT_EQ(result.data(),
            broadcast(makeVariable<double>(Dimensions{Dim::X, 2},
                                           sc_units::counts, Values{3, 4}),
                      Dimensions({Dim::Y, Dim::X}, {5, 2})));
}

TEST(RebinWithMaskTest, preserves_unrelated_mask) {
  Dataset ds({{"data_xy", broadcast(makeVariable<double>(Dimensions{Dim::X, 5},
                                                         sc_units::counts,
                                                         Values{1, 2, 3, 4, 5}),
                                    Dimensions({Dim::Y, Dim::X}, {5, 5}))}},
             {{Dim::X, makeVariable<double>(Dimensions{Dim::X, 6},
                                            Values{1, 2, 3, 4, 5, 6})}});
  ds["data_xy"].masks().set(
      "mask_y", makeVariable<bool>(Dimensions{Dim::Y, 5},
                                   Values{false, false, true, false, false}));

  const auto edges =
      makeVariable<double>(Dimensions{Dim::X, 3}, Values{1, 3, 5});
  const Dataset result = rebin(ds, Dim::X, edges);

  ASSERT_EQ(result["data_xy"].data(),
            broadcast(makeVariable<double>(Dimensions{Dim::X, 2},
                                           sc_units::counts, Values{3, 7}),
                      Dimensions({Dim::Y, Dim::X}, {5, 2})));
  ASSERT_EQ(result["data_xy"].masks()["mask_y"],
            ds["data_xy"].masks()["mask_y"]);
}

TEST_F(RebinTest, rebin_reference_prereserved) {
  auto edges =
      makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1.0, 2.5, 3.5});
  const auto rebinned = rebin(array, Dim::Y, edges);

  EXPECT_EQ(&rebinned.coords()[Dim::Y].values<double>()[0],
            &edges.values<double>()[0]);
}
