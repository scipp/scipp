// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>
#include <vector>

#include "test_macros.h"

#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementArrayView, full_volume) {
  Dimensions dims({{Dim::Y, 4}, {Dim::X, 2}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);
  ElementArrayView<double> view(variable.data(), 0, dims, Strides{dims});
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 8);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 1.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 3.0);
  EXPECT_EQ(*it++, 4.0);
  EXPECT_EQ(*it++, 5.0);
  EXPECT_EQ(*it++, 6.0);
  EXPECT_EQ(*it++, 7.0);
}

TEST(ElementArrayView, subvolume) {
  Dimensions dims({{Dim::Y, 4}, {Dim::X, 2}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  Dimensions variableDims({{Dim::Y, 3}, {Dim::X, 1}});
  ElementArrayView<double> view(variable.data(), 0, variableDims,
                                Strides{dims});
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 3);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 4.0);
}

TEST(ElementArrayView, edges_first) {
  Dimensions dims({{Dim::Y, 4}, {Dim::X, 2}});
  Dimensions edgeDims({{Dim::Y, 4}, {Dim::X, 3}});
  std::vector<double> variable(edgeDims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  ElementArrayView<double> view(variable.data(), 0, dims, Strides{3, 1});
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 8);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 1.0);
  EXPECT_EQ(*it++, 3.0);
  EXPECT_EQ(*it++, 4.0);
  EXPECT_EQ(*it++, 6.0);
  EXPECT_EQ(*it++, 7.0);
  EXPECT_EQ(*it++, 9.0);
  EXPECT_EQ(*it++, 10.0);
}

TEST(ElementArrayView, edges_second) {
  Dimensions dims({{Dim::Y, 4}, {Dim::X, 2}});
  Dimensions edgeDims({{Dim::Y, 5}, {Dim::X, 2}});
  std::vector<double> variable(edgeDims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  ElementArrayView<double> view(variable.data(), 0, dims, Strides{2, 1});
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 8);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 1.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 3.0);
  EXPECT_EQ(*it++, 4.0);
  EXPECT_EQ(*it++, 5.0);
  EXPECT_EQ(*it++, 6.0);
  EXPECT_EQ(*it++, 7.0);
}

TEST(ElementArrayView, subview) {
  Dimensions dims({{Dim::Y, 3}, {Dim::X, 2}});
  std::vector<double> variable(dims.volume());
  std::iota(variable.begin(), variable.end(), 0);

  Dimensions variableDims({{Dim::Y, 3}});
  ElementArrayView<double> view(variable.data(), 0, variableDims, Strides{2});
  auto it = view.begin();
  ASSERT_EQ(std::distance(it, view.end()), 3);
  EXPECT_EQ(*it++, 0.0);
  EXPECT_EQ(*it++, 2.0);
  EXPECT_EQ(*it++, 4.0);
}

auto range(const scipp::index end) {
  std::vector<int32_t> data(end);
  std::iota(data.begin(), data.end(), 0);
  return data;
}

Strides transposed_strides(Dimensions &from, const span<const Dim> order) {
  return core::transpose(Strides{from}, from, order);
}

TEST(ElementArrayViewTest, transpose_2d) {
  Dimensions dims{{Dim::X, Dim::Y}, {2, 3}};
  Dimensions target({Dim::Y, Dim::X}, {3, 2});
  EXPECT_TRUE(
      equals(ElementArrayView(range(6).data(), 0, target,
                              transposed_strides(dims, target.labels())),
             {0, 3, 1, 4, 2, 5}));
}

TEST(ElementArrayViewTest, transpose_3d_yx) {
  Dimensions dims{{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}};
  Dimensions target({Dim::Y, Dim::X, Dim::Z}, {3, 2, 4});
  EXPECT_TRUE(
      equals(ElementArrayView(range(24).data(), 0, target,
                              transposed_strides(dims, target.labels())),
             {0,  1,  2,  3,  12, 13, 14, 15, 4,  5,  6,  7,
              16, 17, 18, 19, 8,  9,  10, 11, 20, 21, 22, 23}));
}

TEST(ElementArrayViewTest, transpose_3d_zy) {
  Dimensions dims{{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}};
  Dimensions target({Dim::X, Dim::Z, Dim::Y}, {2, 4, 3});
  EXPECT_TRUE(
      equals(ElementArrayView(range(24).data(), 0, target,
                              transposed_strides(dims, target.labels())),
             {0,  4,  8,  1,  5,  9,  2,  6,  10, 3,  7,  11,
              12, 16, 20, 13, 17, 21, 14, 18, 22, 15, 19, 23}));
}

TEST(ElementArrayViewTest, transpose_3d_zx) {
  Dimensions dims{{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}};
  Dimensions target({Dim::Z, Dim::Y, Dim::X}, {4, 3, 2});
  EXPECT_TRUE(
      equals(ElementArrayView(range(24).data(), 0, target,
                              transposed_strides(dims, target.labels())),
             {0, 12, 4, 16, 8,  20, 1, 13, 5, 17, 9,  21,
              2, 14, 6, 18, 10, 22, 3, 15, 7, 19, 11, 23}));
}

TEST(ElementArrayViewTest, transpose_3d_zxy) {
  Dimensions dims{{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}};
  Dimensions target({Dim::Z, Dim::X, Dim::Y}, {4, 2, 3});
  EXPECT_TRUE(
      equals(ElementArrayView(range(24).data(), 0, target,
                              transposed_strides(dims, target.labels())),
             {0, 4, 8,  12, 16, 20, 1, 5, 9,  13, 17, 21,
              2, 6, 10, 14, 18, 22, 3, 7, 11, 15, 19, 23}));
}

TEST(ElementArrayViewTest, collapse_inner) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y}, {2, 3});
  Strides strides{3 * 4, 4};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 4, 8, 12, 16, 20}));
  // This is a typical use for the offset parameter.
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 3, target, strides),
                     {3, 7, 11, 15, 19, 23}));
}

TEST(ElementArrayViewTest, collapse_interior) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Z}, {2, 4});
  Strides strides{4 * 3, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 2, 3, 12, 13, 14, 15}));
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 4, target, strides),
                     {4, 5, 6, 7, 16, 17, 18, 19}));
}

TEST(ElementArrayViewTest, collapse_outer) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::Y, Dim::Z}, {3, 4});
  Strides strides{4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
}

TEST(ElementArrayViewTest, collapse_inner_and_outer) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target(Dim::Y, 3);
  Strides strides{4};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 4, 8}));
}

TEST(ElementArrayViewTest, collapse_inner_two) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target(Dim::X, 2);
  Strides strides{3 * 4};
  EXPECT_TRUE(
      equals(ElementArrayView(range(24).data(), 0, target, strides), {0, 12}));
}

TEST(ElementArrayViewTest, collapse_outer_two) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target(Dim::Z, 4);
  Strides strides{1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 2, 3}));
}

TEST(ElementArrayViewTest, collapse_all) {
  Dimensions target;
  Strides strides{};
  EXPECT_TRUE(
      equals(ElementArrayView(range(24).data(), 0, target, strides), {0}));
}

// Note the result of slicing with extent 1 is equivalent to that of collapsing.
TEST(ElementArrayViewTest, slice_inner) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {2, 3, 1});
  Strides strides{3 * 4, 4, 4};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 4, 8, 12, 16, 20}));
  // This is a typical use for the offset parameter.
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 3, target, strides),
                     {3, 7, 11, 15, 19, 23}));
}

TEST(ElementArrayViewTest, slice_interior) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {2, 1, 4});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 2, 3, 12, 13, 14, 15}));
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 4, target, strides),
                     {4, 5, 6, 7, 16, 17, 18, 19}));
}

TEST(ElementArrayViewTest, slice_outer) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {1, 3, 4});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}));
}

TEST(ElementArrayViewTest, slice_inner_and_outer) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {1, 3, 1});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 4, 8}));
}

TEST(ElementArrayViewTest, slice_inner_two) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {2, 1, 1});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(
      equals(ElementArrayView(range(24).data(), 0, target, strides), {0, 12}));
}

TEST(ElementArrayViewTest, slice_outer_two) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {1, 1, 4});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 2, 3}));
}

TEST(ElementArrayViewTest, slice_all) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target{{Dim::X, Dim::Y, Dim::Z}, {1, 1, 1}};
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(
      equals(ElementArrayView(range(24).data(), 0, target, strides), {0}));
}

TEST(ElementArrayViewTest, slice_range_inner) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {2, 3, 2});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21}));
}

TEST(ElementArrayViewTest, slice_range_interior) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {2, 2, 4});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 2, 3, 4, 5, 6, 7, 12, 13, 14, 15, 16, 17, 18, 19}));
}

TEST(ElementArrayViewTest, slice_range_inner_and_outer) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {2, 3, 2});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 4, 5, 8, 9, 12, 13, 16, 17, 20, 21}));
}

TEST(ElementArrayViewTest, slice_range_inner_two) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {2, 2, 2});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 4, 5, 12, 13, 16, 17}));
}

TEST(ElementArrayViewTest, slice_range_outer_two) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target({Dim::X, Dim::Y, Dim::Z}, {1, 2, 4});
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 2, 3, 4, 5, 6, 7}));
}

TEST(ElementArrayViewTest, slice_range_all) {
  // base dims = {{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}}
  Dimensions target{{Dim::X, Dim::Y, Dim::Z}, {1, 2, 2}};
  Strides strides{3 * 4, 4, 1};
  EXPECT_TRUE(equals(ElementArrayView(range(24).data(), 0, target, strides),
                     {0, 1, 4, 5}));
}

TEST(ElementArrayViewTest, view_of_view_bad_broadcast) {
  Dimensions dims1{{Dim::X, Dim::Y}, {2, 2}};
  Dimensions dims2{{Dim::X, Dim::Y}, {2, 3}};
  Strides strides{2, 1};
  const auto data = range(6);
  // Base view with sliced Y
  ElementArrayView base(data.data(), 0, dims1, strides);
  EXPECT_THROW(ElementArrayView<const int32_t>(base, dims2),
               except::DimensionError);
}
