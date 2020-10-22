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

namespace {
template <int N>
constexpr auto make_array_common = [](const auto... values) {
  const auto size = sizeof...(values);
  Variable coord = makeVariable<double>(units::m, Dims{Dim::X}, Shape{size},
                                        Values{values...});
  Variable data = makeVariable<int64_t>(Dims{Dim::X}, Shape{size - N});
  return DataArray{data, {{Dim::X, coord}}};
};
}
constexpr auto make_points = make_array_common<0>;
constexpr auto make_histogram = make_array_common<1>;

TEST(SliceByValueTest, test_dimension_not_found) {
  auto test = [](const auto &sliceable) {
    EXPECT_THROW(auto s = slice(sliceable, Dim::Y, {}, {}),
                 except::NotFoundError);
  };

  auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1.0, 2.0, 3.0, 4.0});
  DataArray da{var, {{Dim::X, var}}};
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_no_multi_dimensional_coords) {
  auto test = [](const auto &sliceable) {
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, {}, {}),
                 except::DimensionError);
  };

  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});
  DataArray da{var, {{Dim::X, var}}};
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_unsorted_coord_throws) {
  auto test = [](const auto &sliceable) {
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, {}, {}), std::runtime_error);
  };
  auto unsorted =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1.0, 2.0, 3.0, 1.5});
  DataArray da{unsorted, {{Dim::X, unsorted}}};
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_begin_end_not_0D_throws) {
  auto da = make_points(0, 1, 2, 3);
  auto one_d = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.0});
  auto test = [&](const auto &sliceable) {
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, one_d, {}),
                 except::MismatchError<Dimensions>);
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, {}, one_d),
                 except::MismatchError<Dimensions>);
  };

  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slicing_defaults_ascending) {
  auto da = make_points(3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(sliceable, slice(sliceable, Dim::X, {}, 13.0 * units::m));
    EXPECT_EQ(sliceable, slice(sliceable, Dim::X, {}, {}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slicing_defaults_descending) {
  auto da = make_points(12, 11, 10, 9, 8, 7, 6, 5, 4, 3);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(sliceable, slice(sliceable, Dim::X, {}, 2.0 * units::m));
    EXPECT_EQ(sliceable, slice(sliceable, Dim::X, {}, {}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slice_range_on_point_coord_1D_ascending) {
  auto da = make_points(3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  auto test = [](const auto &sliceable) {
    // No effect slicing
    auto out = slice(sliceable, Dim::X, 3.0 * units::m, 13.0 * units::m);
    EXPECT_EQ(sliceable, out);
    // Test start on left boundary (closed on left), so includes boundary
    out = slice(sliceable, Dim::X, 3.0 * units::m, 4.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test start out of bounds on left truncated
    out = slice(sliceable, Dim::X, 2.0 * units::m, 4.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test inner values
    out = slice(sliceable, Dim::X, 3.5 * units::m, 5.5 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 1, 3}));
    // Test end on right boundary (open on right), so does not include boundary
    out = slice(sliceable, Dim::X, 11.0 * units::m, 12.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 9}));
    // Test end out of bounds on right truncated
    out = slice(sliceable, Dim::X, 11.0 * units::m, 13.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 10}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slice_range_on_point_coord_1D_descending) {
  auto da = make_points(12, 11, 10, 9, 8, 7, 6, 5, 4, 3);
  auto test = [](const auto &sliceable) {
    // No effect slicing
    auto out = slice(sliceable, Dim::X, 12.0 * units::m, 2.0 * units::m);
    EXPECT_EQ(sliceable, out);
    // Test start on left boundary (closed on left), so includes boundary
    out = slice(sliceable, Dim::X, 12.0 * units::m, 11.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test start out of bounds on left truncated
    out = slice(sliceable, Dim::X, 13.0 * units::m, 11.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test inner values
    out = slice(sliceable, Dim::X, 11.5 * units::m, 9.5 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 1, 3}));
    // Test end on right boundary (open on right), so does not include boundary
    out = slice(sliceable, Dim::X, 4.0 * units::m, 3.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 9}));
    // Test end out of bounds on right truncated
    out = slice(sliceable, Dim::X, 4.0 * units::m, 1.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 10}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slice_range_on_edge_coord_1D_ascending) {
  auto da = make_histogram(3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  const auto test = [](const auto &sliceable) {
    // No effect slicing
    auto out = slice(sliceable, Dim::X, 3.0 * units::m, 13.0 * units::m);
    EXPECT_EQ(out, sliceable);
    // Test start on left boundary (closed on left), so includes boundary
    out = slice(sliceable, Dim::X, 3.0 * units::m, 4.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test slicing with range boundary inside edge, same result as above
    // expected
    out = slice(sliceable, Dim::X, 3.1 * units::m, 4.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test slicing with range lower boundary on upper edge of bin (open on
    // right test)
    out = slice(sliceable, Dim::X, 4.0 * units::m, 6.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 1, 3}));
    // Test end on right boundary (open on right), so does not include boundary
    out = slice(sliceable, Dim::X, 11.0 * units::m, 12.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 9}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slice_range_on_edge_coord_1D_descending) {
  auto da = make_histogram(12, 11, 10, 9, 8, 7, 6, 5, 4, 3);
  // No effect slicing
  auto test = [](const auto &sliceable) {
    auto out = slice(sliceable, Dim::X, 12.0 * units::m, 2.0 * units::m);
    EXPECT_EQ(out, sliceable);
    // Test start on left boundary (closed on left), so includes boundary
    out = slice(sliceable, Dim::X, 12.0 * units::m, 11.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test slicing with range boundary inside edge, same result as above
    // expected
    out = slice(sliceable, Dim::X, 11.9 * units::m, 11.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test slicing with range lower boundary on upper edge of bin (open on
    // right test)
    out = slice(sliceable, Dim::X, 11.0 * units::m, 9.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 1, 3}));
    // Test end on right boundary (open on right), so does not include boundary
    out = slice(sliceable, Dim::X, 4.0 * units::m, 3.0 * units::m);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 9}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_point_on_point_coord_1D) {
  auto da = make_points(1, 3, 5, 4, 2);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 1.0 * units::m),
              sliceable.slice({Dim::X, 0}));
    EXPECT_EQ(slice(sliceable, Dim::X, 3.0 * units::m),
              sliceable.slice({Dim::X, 1}));
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * units::m),
              sliceable.slice({Dim::X, 3}));
    EXPECT_EQ(slice(sliceable, Dim::X, 2.0 * units::m),
              sliceable.slice({Dim::X, 4}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_point_on_point_coord_1D_not_unique) {
  auto da = make_points(1, 3, 5, 3, 2);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 1.0 * units::m),
              sliceable.slice({Dim::X, 0}));
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, 3.0 * units::m),
                 except::SliceError);
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, 4.0 * units::m),
                 except::SliceError);
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slice_point_on_edge_coord_1D) {
  auto da = make_histogram(3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  const auto test = [](const auto &sliceable) {
    // Test start on left boundary (closed on left), so includes boundary
    EXPECT_EQ(slice(sliceable, Dim::X, 3.0 * units::m),
              sliceable.slice({Dim::X, 0}));
    // Same as above, takes lower bounds of bin so same bin
    EXPECT_EQ(slice(sliceable, Dim::X, 3.5 * units::m),
              sliceable.slice({Dim::X, 0}));
    // Next bin
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * units::m),
              sliceable.slice({Dim::X, 1}));
    // Last bin
    EXPECT_EQ(slice(sliceable, Dim::X, 11.9 * units::m),
              sliceable.slice({Dim::X, 8}));
    // (closed on right) so out of bounds
    EXPECT_THROW([[maybe_unused]] auto view =
                     slice(sliceable, Dim::X, 12.0 * units::m),
                 except::SliceError);
    // out of bounds for left for completeness
    EXPECT_THROW([[maybe_unused]] auto view =
                     slice(sliceable, Dim::X, 2.99 * units::m),
                 except::SliceError);
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slice_range_on_point_coord_1D_duplicate) {
  auto da = make_points(3, 4, 4, 5);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * units::m, 4.6 * units::m),
              sliceable.slice({Dim::X, 1, 3}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}

TEST(SliceByValueTest, test_slice_point_on_edge_coord_1D_duplicate) {
  // [4,4) is empty bin, 4 is in [4,5)
  auto da = make_histogram(3, 4, 4, 5);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * units::m),
              sliceable.slice({Dim::X, 2}));
  };
  test(da);                              // Test for DataArray
  test(Dataset{DataArrayConstView{da}}); // Test for Dataset
}
