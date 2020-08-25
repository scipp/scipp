// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

#include "scipp/core/multi_index.h"

using namespace scipp;
using namespace scipp::core;

class MultiIndexTest : public ::testing::Test {
protected:
  void check(MultiIndex<Dimensions> i,
             const std::vector<scipp::index> &indices) const {
    for (const auto index : indices) {
      EXPECT_EQ(i.get(), (std::array<scipp::index, 1>{index}));
      i.increment();
    }
  }
  void check(MultiIndex<Dimensions, Dimensions> i,
             const std::vector<scipp::index> &indices0,
             const std::vector<scipp::index> &indices1) const {
    for (scipp::index n = 0; n < scipp::size(indices0); ++n) {
      EXPECT_EQ(i.get(), (std::array{indices0[n], indices1[n]}));
      i.increment();
    }
  }
  Dimensions x{{Dim::X}, {2}};
  Dimensions yx{{Dim::Y, Dim::X}, {3, 2}};
  Dimensions xy{{Dim::X, Dim::Y}, {2, 3}};
  Dimensions xz{{Dim::X, Dim::Z}, {2, 4}};
  Dimensions xyz{{Dim::X, Dim::Y, Dim::Z}, {2, 3, 4}};
};

TEST_F(MultiIndexTest, to_linear) {
  EXPECT_EQ(to_linear(std::vector{0}, std::vector{2}), 0);
  EXPECT_EQ(to_linear(std::vector{1}, std::vector{2}), 1);
  EXPECT_EQ(to_linear(std::vector{0, 0}, std::vector{1, 1}), 0);
  EXPECT_EQ(to_linear(std::vector{0, 1}, std::vector{2, 2}), 1);
  EXPECT_EQ(to_linear(std::vector{1, 0}, std::vector{2, 2}), 2);
  EXPECT_EQ(to_linear(std::vector{1, 1}, std::vector{2, 2}), 3);
}

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
  check({xy, yx, xy}, {0, 2, 4, 1, 3, 5}, {0, 1, 2, 3, 4, 5});
  check({yx, yx, xy}, {0, 1, 2, 3, 4, 5}, {0, 3, 1, 4, 2, 5});
}
