// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <cmath>
#include <gtest/gtest.h>

#include "scipp/core/except.h"
#include "scipp/core/slice.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/slice.h"
#include "scipp/variable/arithmetic.h"
#include "test_macros.h"

using namespace scipp;
using namespace std::string_literals;

namespace {
template <int N>
constexpr auto make_array_common = [](const auto... values) {
  const auto size = sizeof...(values);
  Variable coord = makeVariable<double>(sc_units::s, Dims{Dim::X}, Shape{size},
                                        Values{values...});
  Variable data = makeVariable<int64_t>(Dims{Dim::X}, Shape{size - N});
  return DataArray{data, {{Dim::X, coord}}};
};
} // namespace
constexpr auto make_points = make_array_common<0>;
constexpr auto make_histogram = make_array_common<1>;

TEST(SliceByValueTest, test_dimension_not_found) {
  auto test = [](const auto &sliceable) {
    EXPECT_THROW(auto s = slice(sliceable, Dim::Y, {}, {}),
                 except::DimensionError);
  };

  auto var =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1.0, 2.0, 3.0, 4.0});
  DataArray da{var, {{Dim::X, var}}};
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_no_multi_dimensional_coords) {
  auto test = [](const auto &sliceable) {
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, {}, {}),
                 except::DimensionError);
  };

  auto var = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 2},
                                  Values{1.0, 2.0, 3.0, 4.0});
  DataArray da{var, {{Dim::X, var}}};
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_unsorted_coord_throws) {
  auto test = [](const auto &sliceable) {
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, {}, {}), std::runtime_error);
  };
  auto unsorted =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1.0, 2.0, 3.0, 1.5});
  DataArray da{unsorted, {{Dim::X, unsorted}}};
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_begin_end_not_0D_throws) {
  auto da = make_points(0, 1, 2, 3);
  auto one_d = makeVariable<double>(Dims{Dim::X}, Shape{1}, Values{1.0});
  auto test = [&](const auto &sliceable) {
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, one_d, {}),
                 except::DimensionError);
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, {}, one_d),
                 except::DimensionError);
  };

  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_unit_mismatch_throws) {
  auto da = make_points(0, 1, 2, 3);
  const auto test = [](const auto &sliceable) {
    EXPECT_THROW_DISCARD(slice(sliceable, Dim::X, 1 * sc_units::m),
                         except::UnitError);
    EXPECT_THROW_DISCARD(slice(sliceable, Dim::X, 1 * sc_units::m, {}),
                         except::UnitError);
    EXPECT_THROW_DISCARD(slice(sliceable, Dim::X, {}, 1 * sc_units::m),
                         except::UnitError);
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_dtype_string) {
  Variable coord = makeVariable<std::string>(
      sc_units::s, Dims{Dim::X}, Shape{3}, Values{"a"s, "b"s, "c"s});
  Variable data = makeVariable<int64_t>(Dims{Dim::X}, Shape{3});
  const DataArray da{data, {{Dim::X, coord}}};

  const auto test = [](const auto &sliceable) {
    const auto x =
        makeVariable<std::string>(sc_units::s, Dims{}, Shape{}, Values{"a"s});
    EXPECT_EQ(slice(sliceable, Dim::X, x), sliceable.slice(Slice{Dim::X, 0}));
    EXPECT_THROW_DISCARD(slice(sliceable, Dim::X, x, {}), except::TypeError);
    EXPECT_THROW_DISCARD(slice(sliceable, Dim::X, {}, x), except::TypeError);
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slicing_defaults_ascending) {
  auto da = make_points(3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(sliceable, slice(sliceable, Dim::X, {}, 13.0 * sc_units::s));
    EXPECT_EQ(sliceable, slice(sliceable, Dim::X, {}, {}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slicing_defaults_descending) {
  auto da = make_points(12, 11, 10, 9, 8, 7, 6, 5, 4, 3);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(sliceable, slice(sliceable, Dim::X, {}, 2.0 * sc_units::s));
    EXPECT_EQ(sliceable, slice(sliceable, Dim::X, {}, {}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_range_on_point_coord_1D_ascending) {
  auto da = make_points(3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  auto test = [](const auto &sliceable) {
    // No effect slicing
    auto out = slice(sliceable, Dim::X, 3.0 * sc_units::s, 13.0 * sc_units::s);
    EXPECT_EQ(sliceable, out);
    // Test start on left boundary (closed on left), so includes boundary
    out = slice(sliceable, Dim::X, 3.0 * sc_units::s, 4.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test start out of bounds on left truncated
    out = slice(sliceable, Dim::X, 2.0 * sc_units::s, 4.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test inner values
    out = slice(sliceable, Dim::X, 3.5 * sc_units::s, 5.5 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 1, 3}));
    // Test end on right boundary (open on right), so does not include boundary
    out = slice(sliceable, Dim::X, 11.0 * sc_units::s, 12.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 9}));
    // Test end out of bounds on right truncated
    out = slice(sliceable, Dim::X, 11.0 * sc_units::s, 13.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 10}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_range_on_point_coord_1D_descending) {
  auto da = make_points(12, 11, 10, 9, 8, 7, 6, 5, 4, 3);
  auto test = [](const auto &sliceable) {
    // No effect slicing
    auto out = slice(sliceable, Dim::X, 12.0 * sc_units::s, 2.0 * sc_units::s);
    EXPECT_EQ(sliceable, out);
    // Test start on left boundary (closed on left), so includes boundary
    out = slice(sliceable, Dim::X, 12.0 * sc_units::s, 11.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test start out of bounds on left truncated
    out = slice(sliceable, Dim::X, 13.0 * sc_units::s, 11.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test inner values
    out = slice(sliceable, Dim::X, 11.5 * sc_units::s, 9.5 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 1, 3}));
    // Test end on right boundary (open on right), so does not include boundary
    out = slice(sliceable, Dim::X, 4.0 * sc_units::s, 3.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 9}));
    // Test end out of bounds on right truncated
    out = slice(sliceable, Dim::X, 4.0 * sc_units::s, 1.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 10}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_range_on_edge_coord_1D_ascending) {
  auto da = make_histogram(3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  const auto test = [](const auto &sliceable) {
    // No effect slicing
    EXPECT_EQ(slice(sliceable, Dim::X, 3.0 * sc_units::s, 12.0 * sc_units::s),
              sliceable);
    // Test start on left boundary (closed on left), so includes boundary
    EXPECT_EQ(slice(sliceable, Dim::X, 3.0 * sc_units::s, 4.0 * sc_units::s),
              sliceable.slice({Dim::X, 0, 1}));
    // Left range boundary inside edge, same result as above
    EXPECT_EQ(slice(sliceable, Dim::X, 3.1 * sc_units::s, 4.0 * sc_units::s),
              sliceable.slice({Dim::X, 0, 1}));
    // Right range boundary inside edge, same result as above
    EXPECT_EQ(slice(sliceable, Dim::X, 3.1 * sc_units::s, 3.9 * sc_units::s),
              sliceable.slice({Dim::X, 0, 1}));
    // New bound gets included if stop == bound + epsilon
    EXPECT_EQ(slice(sliceable, Dim::X, 3.1 * sc_units::s,
                    std::nextafter(4.0, 9.0) * sc_units::s),
              sliceable.slice({Dim::X, 0, 2}));
    // Test slicing with range lower boundary on upper edge of bin (open on
    // right test): The bin containing the stop value is *included*
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * sc_units::s, 6.0 * sc_units::s),
              sliceable.slice({Dim::X, 1, 3}));
    // Test end on right boundary (open on right), so does not include boundary
    EXPECT_EQ(slice(sliceable, Dim::X, 11.0 * sc_units::s, 12.0 * sc_units::s),
              sliceable.slice({Dim::X, 8, 9}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_range_on_edge_coord_1D_descending) {
  auto da = make_histogram(12, 11, 10, 9, 8, 7, 6, 5, 4, 3);
  // No effect slicing
  auto test = [](const auto &sliceable) {
    auto out = slice(sliceable, Dim::X, 12.0 * sc_units::s, 3.0 * sc_units::s);
    EXPECT_EQ(out, sliceable);
    // Test start on left boundary (closed on left), so includes boundary
    out = slice(sliceable, Dim::X, 12.0 * sc_units::s, 11.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test slicing with range boundary inside edge, same result as above
    // expected
    out = slice(sliceable, Dim::X, 11.9 * sc_units::s, 11.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 0, 1}));
    // Test slicing with range lower boundary on upper edge of bin (open on
    // right test)
    out = slice(sliceable, Dim::X, 11.0 * sc_units::s, 9.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 1, 3}));
    // Test end on right boundary (open on right), so does not include boundary
    out = slice(sliceable, Dim::X, 4.0 * sc_units::s, 3.0 * sc_units::s);
    EXPECT_EQ(out, sliceable.slice({Dim::X, 8, 9}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_point_on_point_coord_1D) {
  auto da = make_points(1, 3, 5, 4, 2);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 1.0 * sc_units::s),
              sliceable.slice({Dim::X, 0}));
    EXPECT_EQ(slice(sliceable, Dim::X, 3.0 * sc_units::s),
              sliceable.slice({Dim::X, 1}));
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * sc_units::s),
              sliceable.slice({Dim::X, 3}));
    EXPECT_EQ(slice(sliceable, Dim::X, 2.0 * sc_units::s),
              sliceable.slice({Dim::X, 4}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_point_on_point_coord_1D_not_unique) {
  auto da = make_points(1, 3, 5, 3, 2);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 1.0 * sc_units::s),
              sliceable.slice({Dim::X, 0}));
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, 3.0 * sc_units::s),
                 except::SliceError);
    EXPECT_THROW(auto s = slice(sliceable, Dim::X, 4.0 * sc_units::s),
                 except::SliceError);
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_point_on_edge_coord_1D) {
  auto da = make_histogram(3, 4, 5, 6, 7, 8, 9, 10, 11, 12);
  const auto test = [](const auto &sliceable) {
    // Test start on left boundary (closed on left), so includes boundary
    EXPECT_EQ(slice(sliceable, Dim::X, 3.0 * sc_units::s),
              sliceable.slice({Dim::X, 0}));
    // Same as above, takes lower bounds of bin so same bin
    EXPECT_EQ(slice(sliceable, Dim::X, 3.5 * sc_units::s),
              sliceable.slice({Dim::X, 0}));
    // Next bin
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * sc_units::s),
              sliceable.slice({Dim::X, 1}));
    // Last bin
    EXPECT_EQ(slice(sliceable, Dim::X, 11.9 * sc_units::s),
              sliceable.slice({Dim::X, 8}));
    // (closed on right) so out of bounds
    EXPECT_THROW([[maybe_unused]] auto view =
                     slice(sliceable, Dim::X, 12.0 * sc_units::s),
                 except::SliceError);
    // out of bounds for left for completeness
    EXPECT_THROW([[maybe_unused]] auto view =
                     slice(sliceable, Dim::X, 2.99 * sc_units::s),
                 except::SliceError);
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_range_on_point_coord_1D_duplicate) {
  auto da = make_points(3, 4, 4, 5);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * sc_units::s, 4.6 * sc_units::s),
              sliceable.slice({Dim::X, 1, 3}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_point_on_edge_coord_1D_duplicate) {
  // [4,4) is empty bin, 4 is in [4,5)
  auto da = make_histogram(3, 4, 4, 5);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 4.0 * sc_units::s),
              sliceable.slice({Dim::X, 2}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_point_on_single_point_1D) {
  auto da = make_points(2);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 2 * sc_units::s),
              sliceable.slice({Dim::X, 0}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_point_on_single_edge_1D) {
  auto da = make_histogram(2, 3);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 2 * sc_units::s),
              sliceable.slice({Dim::X, 0}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_range_on_single_point_1D) {
  auto da = make_points(2);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 2 * sc_units::s, 3 * sc_units::s),
              sliceable.slice({Dim::X, 0, 1}));
  };
  test(da);
  test(Dataset{da});
}

TEST(SliceByValueTest, test_slice_range_on_single_edge_1D) {
  auto da = make_histogram(2, 3);
  const auto test = [](const auto &sliceable) {
    EXPECT_EQ(slice(sliceable, Dim::X, 2 * sc_units::s, 3 * sc_units::s),
              sliceable.slice({Dim::X, 0, 1}));
  };
  test(da);
  test(Dataset{da});
}
