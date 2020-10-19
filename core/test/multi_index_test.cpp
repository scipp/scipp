// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

#include "scipp/core/element_array_view.h"
#include "scipp/core/except.h"
#include "scipp/core/multi_index.h"

using namespace scipp;
using namespace scipp::core;

class MultiIndexTest : public ::testing::Test {
protected:
  template <scipp::index N, class... Indices>
  void check_impl(MultiIndex<N> i, const std::vector<scipp::index> &indices0,
                  const Indices &... indices) const {
    if (scipp::size(indices0) > 0) {
      ASSERT_NE(i.begin(), i.end());
    }
    const bool skip_set_index_check = i != i.begin();
    for (scipp::index n = 0; n < scipp::size(indices0); ++n) {
      EXPECT_EQ(i.get(), (std::array{indices0[n], indices[n]...}));
      i.increment();
    }
    ASSERT_EQ(i, i.end());
    if (skip_set_index_check)
      return;
    if (i.end_sentinel() == scipp::size(indices0)) {
      // No buckets
      for (scipp::index n0 = 0; n0 < scipp::size(indices0); ++n0) {
        i.set_index(n0);
        for (scipp::index n = n0; n < scipp::size(indices0); ++n) {
          EXPECT_EQ(i.get(), (std::array{indices0[n], indices[n]...}));
          i.increment();
        }
      }
    } else {
      // Buckets
      for (scipp::index bucket = 0; bucket < i.end_sentinel(); ++bucket) {
        i.set_index(bucket);
        scipp::index n0 = 0;
        auto it = i.begin();
        while (it != i) {
          it.increment();
          ++n0;
        }
        i.set_index(bucket);
        for (scipp::index n = n0; n < scipp::size(indices0); ++n) {
          EXPECT_EQ(i.get(), (std::array{indices0[n], indices[n]...}))
              << bucket << ' ' << n0;
          i.increment();
        }
      }
    }
  }
  void check(MultiIndex<1> i, const std::vector<scipp::index> &indices) const {
    check_impl(i, indices);
  }
  void check(MultiIndex<2> i, const std::vector<scipp::index> &indices0,
             const std::vector<scipp::index> &indices1) const {
    check_impl(i, indices0, indices1);
  }
  void check_with_buckets(
      const Dimensions &bufferDims, const Dim sliceDim,
      const std::vector<std::pair<scipp::index, scipp::index>> &indices,
      const Dimensions &iterDims, const Dimensions &dataDims,
      const std::vector<scipp::index> &expected) {
    BucketParams params{sliceDim, bufferDims, indices.data()};
    MultiIndex<1> index(element_array_view{0, iterDims, dataDims, params});
    check(index, expected);
  }
  void check_with_buckets(
      const Dimensions &bufferDims0, const Dim sliceDim0,
      const std::vector<std::pair<scipp::index, scipp::index>> &indices0,
      const Dimensions &bufferDims1, const Dim sliceDim1,
      const std::vector<std::pair<scipp::index, scipp::index>> &indices1,
      const Dimensions &iterDims, const Dimensions &dataDims0,
      const Dimensions &dataDims1, const std::vector<scipp::index> &expected0,
      const std::vector<scipp::index> &expected1) {
    BucketParams params0{sliceDim0, bufferDims0, indices0.data()};
    BucketParams params1{sliceDim1, bufferDims1, indices1.data()};
    MultiIndex<2> index(element_array_view{0, iterDims, dataDims0, params0},
                        element_array_view{0, iterDims, dataDims1, params1});
    check(index, expected0, expected1);
    // Order of arguments should not matter, in particular this also tests that
    // the dense argument may be the first argument.
    MultiIndex<2> swapped(element_array_view{0, iterDims, dataDims1, params1},
                          element_array_view{0, iterDims, dataDims0, params0});
    check(swapped, expected1, expected0);
  }

  Dimensions x{{Dim::X}, {2}};
  Dimensions y{{Dim::Y}, {3}};
  Dimensions yx{{Dim::Y, Dim::X}, {3, 2}};
  Dimensions xy{{Dim::X, Dim::Y}, {2, 3}};
  Dimensions xz{{Dim::X, Dim::Z}, {2, 4}};
  Dimensions xyz{{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}};
};

namespace {
constexpr static auto check_strides =
    [](const Dimensions &iter, const Dimensions &data,
       const std::vector<scipp::index> &expected) {
      std::array<scipp::index, NDIM_MAX> array = {};
      std::copy_n(expected.begin(), expected.size(), array.begin());
      EXPECT_EQ(get_strides(iter, data), array);
    };
}

TEST_F(MultiIndexTest, get_strides) {
  check_strides({Dim::X, 1}, {Dim::X, 1}, {1});
  check_strides({Dim::X, 2}, {Dim::X, 2}, {1});
  // Y sliced out, broadcast slice to X
  check_strides({Dim::X, 2}, {Dim::Y, 2}, {0});
  // Note that internally order is reversed
  check_strides(yx, yx, {1, 2});
  check_strides(xy, yx, {2, 1});
}

TEST_F(MultiIndexTest, broadcast_inner) { check({xy, x}, {0, 0, 0, 1, 1, 1}); }

TEST_F(MultiIndexTest, broadcast_outer) { check({yx, x}, {0, 1, 0, 1, 0, 1}); }

TEST_F(MultiIndexTest, slice_inner) { check({x, xy}, {0, 3}); }

TEST_F(MultiIndexTest, slice_middle) {
  check({xz, xyz}, {0, 1, 2, 3, 12, 13, 14, 15});
}

TEST_F(MultiIndexTest, slice_outer) { check({x, yx}, {0, 1}); }

TEST_F(MultiIndexTest, 2d) { check({xy, xy}, {0, 1, 2, 3, 4, 5}); }

TEST_F(MultiIndexTest, 2d_transpose) { check({yx, xy}, {0, 3, 1, 4, 2, 5}); }

TEST_F(MultiIndexTest, slice_and_broadcast) {
  check({xz, yx}, {0, 0, 0, 0, 1, 1, 1, 1});
  check({xz, xy}, {0, 0, 0, 0, 3, 3, 3, 3});
  check({yx, xz}, {0, 4, 0, 4, 0, 4});
}

TEST_F(MultiIndexTest, multiple_data_indices) {
  check({yx, x, y}, {0, 1, 0, 1, 0, 1}, {0, 0, 1, 1, 2, 2});
  check({xy, x, y}, {0, 0, 0, 1, 1, 1}, {0, 1, 2, 0, 1, 2});
  check({xy, yx, xy}, {0, 2, 4, 1, 3, 5}, {0, 1, 2, 3, 4, 5});
  check({yx, yx, xy}, {0, 1, 2, 3, 4, 5}, {0, 3, 1, 4, 2, 5});
}

TEST_F(MultiIndexTest, advance_multiple_data_indices) {
  MultiIndex index(yx, x, y);
  index.set_index(1);
  check(index, {1, 0, 1, 0, 1}, {0, 1, 1, 2, 2});
  index.set_index(2);
  check(index, {0, 1, 0, 1}, {1, 1, 2, 2});
}

TEST_F(MultiIndexTest, advance_slice_middle) {
  MultiIndex index(xz, xyz);
  index.set_index(2);
  check(index, {2, 3, 12, 13, 14, 15});
  index.set_index(5);
  check(index, {13, 14, 15});
}

TEST_F(MultiIndexTest, advance_slice_and_broadcast) {
  MultiIndex index(xz, xy);
  index.set_index(2);
  check(index, {0, 0, 3, 3, 3, 3});
}

TEST_F(MultiIndexTest, 1d_array_of_1d_buckets) {
  const Dim dim = Dim::Row;
  Dimensions buf{{dim}, {7}}; // 1d cut into two sections
  // natural order no gaps
  check_with_buckets(buf, dim, {{0, 3}, {3, 7}}, x, x, {0, 1, 2, 3, 4, 5, 6});
  // gap between
  check_with_buckets(buf, dim, {{0, 3}, {4, 7}}, x, x, {0, 1, 2, 4, 5, 6});
  // gap at start
  check_with_buckets(buf, dim, {{1, 3}, {3, 7}}, x, x, {1, 2, 3, 4, 5, 6});
  // out of order
  check_with_buckets(buf, dim, {{4, 7}, {0, 4}}, x, x, {4, 5, 6, 0, 1, 2, 3});
}

TEST_F(MultiIndexTest, 1d_array_of_2d_buckets) {
  Dimensions buf{{Dim("a"), Dim("b")}, {2, 3}}; // 2d cut into two sections
  // cut along inner
  check_with_buckets(buf, Dim("b"), {{0, 1}, {1, 3}}, x, x, {0, 3, 1, 2, 4, 5});
  check_with_buckets(buf, Dim("b"), {{0, 1}, {2, 3}}, x, x, {0, 3, 2, 5});
  check_with_buckets(buf, Dim("b"), {{1, 2}, {2, 3}}, x, x, {1, 4, 2, 5});
  check_with_buckets(buf, Dim("b"), {{1, 3}, {0, 1}}, x, x, {1, 2, 4, 5, 0, 3});
  // cut along outer
  check_with_buckets(buf, Dim("a"), {{0, 1}, {1, 2}}, x, x, {0, 1, 2, 3, 4, 5});
  check_with_buckets(buf, Dim("a"), {{1, 2}, {1, 2}}, x, x, {3, 4, 5, 3, 4, 5});
  check_with_buckets(buf, Dim("a"), {{1, 2}, {0, 1}}, x, x, {3, 4, 5, 0, 1, 2});
}

TEST_F(MultiIndexTest, 2d_array_of_1d_buckets) {
  const Dim dim = Dim::Row;
  Dimensions buf{{dim}, {12}}; // 1d cut into xy=2x3 sections
  check_with_buckets(buf, dim,
                     {{0, 2}, {2, 4}, {4, 6}, {6, 8}, {8, 10}, {10, 12}}, xy,
                     xy, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11});
  check_with_buckets(buf, dim,
                     {{1, 2}, {2, 4}, {5, 6}, {6, 8}, {8, 10}, {10, 12}}, xy,
                     xy, {1, 2, 3, 5, 6, 7, 8, 9, 10, 11});
  // transpose
  check_with_buckets(buf, dim,
                     {{0, 2}, {2, 4}, {4, 6}, {6, 8}, {8, 10}, {10, 12}}, yx,
                     xy, {0, 1, 6, 7, 2, 3, 8, 9, 4, 5, 10, 11});
  // slice inner
  check_with_buckets(buf, dim,
                     {{0, 2}, {2, 4}, {4, 6}, {6, 8}, {8, 10}, {10, 12}}, x, xy,
                     {0, 1, 6, 7});
  // slice outer
  check_with_buckets(buf, dim,
                     {{0, 2}, {2, 4}, {4, 6}, {6, 8}, {8, 10}, {10, 12}}, y, xy,
                     {0, 1, 2, 3, 4, 5});
}

TEST_F(MultiIndexTest, 1d_array_of_1d_buckets_and_dense) {
  const Dim dim = Dim::Row;
  Dimensions buf{{dim}, {7}}; // 1d cut into two sections
  // natural order no gaps
  check_with_buckets(buf, dim, {{0, 3}, {3, 7}}, Dimensions{}, Dim::Invalid, {},
                     x, x, x, {0, 1, 2, 3, 4, 5, 6}, {0, 0, 0, 1, 1, 1, 1});
  // gap between
  check_with_buckets(buf, dim, {{0, 3}, {4, 7}}, Dimensions{}, Dim::Invalid, {},
                     x, x, x, {0, 1, 2, 4, 5, 6}, {0, 0, 0, 1, 1, 1});
  // gap at start
  check_with_buckets(buf, dim, {{1, 3}, {3, 7}}, Dimensions{}, Dim::Invalid, {},
                     x, x, x, {1, 2, 3, 4, 5, 6}, {0, 0, 1, 1, 1, 1});
  // out of order
  // Note that out of order bucket indices is *not* to be confused with
  // reversing a dimension, i.e., we do *not* expect {1,1,1,0,0,0,0} for the
  // dense part.
  check_with_buckets(buf, dim, {{4, 7}, {0, 4}}, Dimensions{}, Dim::Invalid, {},
                     x, x, x, {4, 5, 6, 0, 1, 2, 3}, {0, 0, 0, 1, 1, 1, 1});
}

TEST_F(MultiIndexTest, 1d_array_of_1d_buckets_and_dense_with_empty_buckets) {
  const Dim dim = Dim::Row;
  Dimensions buf{{dim}, {7}};
  Dimensions x1{{Dim::X}, {1}};
  check_with_buckets(buf, dim, {{0, 0}}, Dimensions{}, Dim::Invalid, {}, x1, x1,
                     x1, {}, {});
  check_with_buckets(buf, dim, {{1, 1}, {0, 0}}, Dimensions{}, Dim::Invalid, {},
                     x, x, x, {}, {});
  check_with_buckets(buf, dim, {{0, 0}, {0, 3}}, Dimensions{}, Dim::Invalid, {},
                     x, x, x, {0, 1, 2}, {1, 1, 1});
  check_with_buckets(buf, dim, {{0, 2}, {2, 2}, {3, 5}}, Dimensions{},
                     Dim::Invalid, {}, y, y, y, {0, 1, 3, 4}, {0, 0, 2, 2});
  check_with_buckets(buf, dim, {{0, 2}, {3, 5}, {5, 5}}, Dimensions{},
                     Dim::Invalid, {}, y, y, y, {0, 1, 3, 4}, {0, 0, 1, 1});
}

TEST_F(MultiIndexTest, two_1d_arrays_of_1d_buckets) {
  const Dim dim = Dim::Row;
  Dimensions buf{{dim}, {1}};
  check_with_buckets(buf, dim, {{0, 3}, {3, 7}}, buf, dim, {{4, 7}, {0, 4}}, x,
                     x, x, {0, 1, 2, 3, 4, 5, 6}, {4, 5, 6, 0, 1, 2, 3});
  // slice inner
  check_with_buckets(buf, dim, {{0, 3}, {3, 7}}, buf, dim,
                     {{1, 4}, {5, 9}, {9, 10}, {10, 11}, {11, 12}, {12, 13}}, x,
                     x, yx, {0, 1, 2, 3, 4, 5, 6}, {1, 2, 3, 5, 6, 7, 8});
  // slice outer
  check_with_buckets(buf, dim, {{0, 3}, {3, 7}}, buf, dim,
                     {{1, 4}, {9, 10}, {10, 11}, {5, 9}, {11, 12}, {12, 13}}, x,
                     x, xy, {0, 1, 2, 3, 4, 5, 6}, {1, 2, 3, 5, 6, 7, 8});
  // slice to scalar
  check_with_buckets(buf, dim, {{0, 3}}, buf, dim, {{2, 5}, {0, 2}},
                     Dimensions{}, x, x, {0, 1, 2}, {2, 3, 4});
}

TEST_F(MultiIndexTest, two_1d_arrays_of_1d_buckets_bucket_size_mismatch) {
  const Dim dim = Dim::Row;
  Dimensions buf{{dim}, {7}};
  EXPECT_THROW(check_with_buckets(buf, dim, {{0, 3}, {3, 7}}, buf, dim,
                                  {{0, 4}, {3, 7}}, x, x, x,
                                  {0, 1, 2, 3, 4, 5, 6}, {0, 1, 2, 3, 4, 5, 6}),
               except::BucketError);
  EXPECT_THROW(check_with_buckets(buf, dim, {{0, 3}, {3, 7}}, buf, dim,
                                  {{0, 3}, {4, 7}}, x, x, x,
                                  {0, 1, 2, 3, 4, 5, 6}, {0, 1, 2, 3, 4, 5, 6}),
               except::BucketError);
}
