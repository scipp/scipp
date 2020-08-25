// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/core/slice.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/slice.h"
#include "scipp/variable/arithmetic.h"
#include "test_macros.h"

using namespace scipp;

TEST(SliceByValueTest, test_dimension_not_found) {
  auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1.0, 2.0, 3.0, 4.0});
  DataArray da{var, {{Dim::X, var}}};
  EXPECT_THROW(auto s = slice(da, Dim::Y), except::NotFoundError);
}

enum class CoordType { BinEdges, Points };
DataArray make_1d_data_array(scipp::index begin, scipp::index end, Dim dim,
                             CoordType coord_type) {
  auto size = std::abs(begin - end);
  double step = end > begin ? 1.0 : -1.0;
  element_array<double> values(size);
  for (scipp::index i = 0; i < size; ++i) {
    values.data()[i] = begin + (i * step);
  }
  Variable coord(units::one, Dimensions{{dim, size}}, values,
                 std::optional<element_array<double>>{});
  Variable data(
      units::one,
      Dimensions{{dim, coord_type == CoordType::BinEdges ? size - 1 : size}},
      element_array<double>(values.begin(), coord_type == CoordType::BinEdges
                                                ? values.end() - 1
                                                : values.end()),
      std::optional<element_array<double>>{});
  return DataArray{data, {{dim, coord}}};
}

TEST(SliceByValueTest, test_no_multi_dimensional_coords) {
  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});
  DataArray da{var, {{Dim::X, var}}};
  EXPECT_THROW(auto s = slice(da, Dim::X), except::DimensionError);
}

TEST(SliceByValueTest, test_unsorted_coord_throws) {
  auto unsorted =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1.0, 2.0, 3.0, 1.5});
  DataArray da{unsorted, {{Dim::X, unsorted}}};
  EXPECT_THROW(auto s = slice(da, Dim::X), std::runtime_error);
}

TEST(SliceByValueTest, test_begin_end_not_0D_throws) {
  auto da = make_1d_data_array(0, 3, Dim::X, CoordType::Points);
  auto one_d = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.0});
  EXPECT_THROW(auto s = slice(da, Dim::X, one_d, VariableConstView{}),
               except::MismatchError<Dimensions>);
  EXPECT_THROW(auto s = slice(da, Dim::X, VariableConstView{}, one_d),
               except::MismatchError<Dimensions>);
}

TEST(SliceByValueTest, test_slicing_defaults_ascending) {
  auto da = make_1d_data_array(3, 13, Dim::X, CoordType::Points);
  EXPECT_EQ(da, slice(da, Dim::X, VariableConstView{}, 13.0 * units::one));
  EXPECT_EQ(da.slice({Dim::X, 0, 9}),
            slice(da, Dim::X)); // Note open on the right with default end
}

TEST(SliceByValueTest, test_slicing_defaults_descending) {
  auto da = make_1d_data_array(12, 2, Dim::X, CoordType::Points);
  EXPECT_EQ(da, slice(da, Dim::X, VariableConstView{}, 2.0 * units::one));
  EXPECT_EQ(da.slice({Dim::X, 0, 9}),
            slice(da, Dim::X)); // Note open on the right with default end
}

TEST(SliceByValueTest, test_slice_range_on_point_coords_1D_ascending) {
  //    Data Values           [0.0][1.0] ... [8.0][9.0]
  //    Coord Values (points) [3.0][4.0] ... [11.0][12.0]

  auto da = make_1d_data_array(3, 13, Dim::X, CoordType::Points);
  // No effect slicing
  auto out = slice(da, Dim::X, 3.0 * units::one, 13.0 * units::one);
  EXPECT_EQ(da, out);
  // Test start on left boundary (closed on left), so includes boundary
  out = slice(da, Dim::X, 3.0 * units::one, 4.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 0, 1}));
  // Test start out of bounds on left truncated
  out = slice(da, Dim::X, 2.0 * units::one, 4.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 0, 1}));
  // Test inner values
  out = slice(da, Dim::X, 3.5 * units::one, 5.5 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 1, 3}));
  // Test end on right boundary (open on right), so does not include boundary
  out = slice(da, Dim::X, 11.0 * units::one, 12.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 8, 9}));
  // Test end out of bounds on right truncated
  out = slice(da, Dim::X, 11.0 * units::one, 13.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 8, 10}));
}

TEST(SliceByValueTest, test_slice_range_on_point_coords_1D_descending) {
  //    Data Values           [0.0][1.0] ... [8.0][9.0]
  //    Coord Values (points) [12.0][11.0] ... [4.0][3.0]

  auto da = make_1d_data_array(12, 2, Dim::X, CoordType::Points);
  // No effect slicing
  auto out = slice(da, Dim::X, 12.0 * units::one, 2.0 * units::one);
  EXPECT_EQ(da, out);
  // Test start on left boundary (closed on left), so includes boundary
  out = slice(da, Dim::X, 12.0 * units::one, 11.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 0, 1}));
  // Test start out of bounds on left truncated
  out = slice(da, Dim::X, 13.0 * units::one, 11.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 0, 1}));
  // Test inner values
  out = slice(da, Dim::X, 11.5 * units::one, 9.5 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 1, 3}));
  // Test end on right boundary (open on right), so does not include boundary
  out = slice(da, Dim::X, 4.0 * units::one, 3.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 8, 9}));
  // Test end out of bounds on right truncated
  out = slice(da, Dim::X, 4.0 * units::one, 1.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 8, 10}));
}

TEST(SliceByValueTest, test_slice_range_on_edge_coords_1D_ascending) {
  //    Data Values            [0.0] ...       [8.0]
  //    Coord Values (edges) [3.0][4.0] ... [11.0][12.0]
  auto da = make_1d_data_array(3, 13, Dim::X, CoordType::BinEdges);
  // No effect slicing
  auto out = slice(da, Dim::X, 3.0 * units::one, 13.0 * units::one);
  EXPECT_EQ(out, da);
  // Test start on left boundary (closed on left), so includes boundary
  out = slice(da, Dim::X, 3.0 * units::one, 4.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 0, 1}));
  // Test slicing with range boundary inside edge, same result as above expected
  out = slice(da, Dim::X, 3.1 * units::one, 4.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 0, 1}));
  // Test slicing with range lower boundary on upper edge of bin (open on right
  // test)
  out = slice(da, Dim::X, 4.0 * units::one, 6.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 1, 3}));
  // Test end on right boundary (open on right), so does not include boundary
  out = slice(da, Dim::X, 11.0 * units::one, 12.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 8, 9}));
}

TEST(SliceByValueTest, test_slice_range_on_edge_coords_1D_descending) {
  //    Data Values            [0.0] ...       [8.0]
  //    Coord Values (edges) [12.0][11.0] ... [4.0][3.0]
  auto da = make_1d_data_array(12, 2, Dim::X, CoordType::BinEdges);
  // No effect slicing
  auto out = slice(da, Dim::X, 12.0 * units::one, 2.0 * units::one);
  EXPECT_EQ(out, da);
  // Test start on left boundary (closed on left), so includes boundary
  out = slice(da, Dim::X, 12.0 * units::one, 11.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 0, 1}));
  // Test slicing with range boundary inside edge, same result as above expected
  out = slice(da, Dim::X, 11.9 * units::one, 11.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 0, 1}));
  // Test slicing with range lower boundary on upper edge of bin (open on right
  // test)
  out = slice(da, Dim::X, 11.0 * units::one, 9.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 1, 3}));
  // Test end on right boundary (open on right), so does not include boundary
  out = slice(da, Dim::X, 4.0 * units::one, 3.0 * units::one);
  EXPECT_EQ(out, da.slice({Dim::X, 8, 9}));
}

TEST(SliceByValueTest, test_point_on_point_coords_1D_ascending) {
  //    Data Values           [0.0][1.0] ... [8.0][9.0]
  //    Coord Values (points) [3.0][4.0] ... [11.0][12.0]

  auto da = make_1d_data_array(3, 13, Dim::X, CoordType::Points);
  // No effect slicing
  // Test start on left boundary (closed on left), so includes boundary
  auto begin = 3.0 * units::one;
  auto out = slice(da, Dim::X, begin, begin);
  EXPECT_EQ(out, da.slice({Dim::X, 0}));
  // Test point slice between points throws
  begin = 3.5 * units::one;
  EXPECT_THROW(auto s = slice(da, Dim::X, begin, begin), except::NotFoundError);
  // Test start on right boundary
  begin = 12.0 * units::one;
  out = slice(da, Dim::X, begin, begin);
  EXPECT_EQ(out, da.slice({Dim::X, 9}));
  // Test start outside right boundary throws
  begin = 12.1 * units::one;
  EXPECT_THROW(auto s = slice(da, Dim::X, begin, begin), except::NotFoundError);
}

TEST(SliceByValueTest, test_point_on_point_coords_1D_descending) {
  //    Data Values           [0.0][1.0] ... [8.0][9.0]
  //    Coord Values (points) [12.0][11.0] ... [3.0][2.0]

  auto da = make_1d_data_array(12, 2, Dim::X, CoordType::Points);
  // No effect slicing
  // Test start on left boundary (closed on left), so includes boundary
  auto begin = 12.0 * units::one;
  auto out = slice(da, Dim::X, begin, begin);
  EXPECT_EQ(out, da.slice({Dim::X, 0}));
  // Test point slice between points throws
  begin = 3.5 * units::one;
  EXPECT_THROW(auto s = slice(da, Dim::X, begin, begin), except::NotFoundError);
  // Test start on right boundary
  begin = 3.0 * units::one;
  out = slice(da, Dim::X, begin, begin);
  EXPECT_EQ(out, da.slice({Dim::X, 9}));
  // Test start outside right boundary throws
  begin = 2.99 * units::one;
  EXPECT_THROW(auto s = slice(da, Dim::X, begin, begin), except::NotFoundError);
}

TEST(SliceByValueTest, test_slice_point_on_edge_coords_1D) {
  //    Data Values              [0.0] ... [8.0]
  //    Coord Values (points) [3.0][4.0] ... [11.0][12.0]

  auto da = make_1d_data_array(3, 13, Dim::X, CoordType::BinEdges);

  // test no-effect slicing
  // Test start on left boundary (closed on left), so includes boundary
  auto begin = 3.0 * units::one;
  auto out = slice(da, Dim::X, begin, begin);
  EXPECT_EQ(out, da.slice({Dim::X, 0}));
  // Same as above, takes lower bounds of bin so same bin
  begin = 3.5 * units::one;
  out = slice(da, Dim::X, begin, begin);
  EXPECT_EQ(out, da.slice({Dim::X, 0}));
  // Next bin
  begin = 4.0 * units::one;
  out = slice(da, Dim::X, begin, begin);
  EXPECT_EQ(out, da.slice({Dim::X, 1}));
  // Last bin
  begin = 11.9 * units::one;
  out = slice(da, Dim::X, begin, begin);
  EXPECT_EQ(out, da.slice({Dim::X, 8}));
  // (closed on right) so out of bounds
  begin = 12.0 * units::one;
  EXPECT_THROW(auto s = slice(da, Dim::X, begin, begin), except::NotFoundError);
  // out of bounds for left for completeness
  begin = 2.99 * units::one;
  EXPECT_THROW(auto s = slice(da, Dim::X, begin, begin), except::NotFoundError);
}
