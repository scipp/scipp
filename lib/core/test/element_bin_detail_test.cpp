// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#include "scipp/core/element/bin_detail.h"

#include <gtest/gtest.h>
#include <vector>

using namespace scipp;
using namespace scipp::core::element;

template <typename T> class ElementBinUtilTest : public ::testing::Test {};
using CoordEdgeTypePairs =
    ::testing::Types<std::pair<double, double>, std::pair<double, float>,
                     std::pair<double, int32_t>, std::pair<double, int64_t>,
                     std::pair<float, double>, std::pair<float, float>,
                     std::pair<float, int32_t>, std::pair<float, int64_t>>;
TYPED_TEST_SUITE(ElementBinUtilTest, CoordEdgeTypePairs);

TYPED_TEST(ElementBinUtilTest, begin_edge) {
  typedef typename TypeParam::first_type CoordType;
  typedef typename TypeParam::second_type EdgeType;
  constexpr auto check = [](const CoordType coord,
                            const scipp::index expected) {
    const std::vector<EdgeType> edges{0, 1, 2, 3};
    scipp::index bin{0};
    scipp::index index;
    begin_edge(bin, index, coord, edges);
    EXPECT_EQ(index, expected);
  };
  check(-0.1, 0);
  check(0.0, 0);
  check(1.0, 1);
  check(1.5, 1);
  check(2.0, 2);
  check(2.5, 2);
  check(3.0, 2);
}

TYPED_TEST(ElementBinUtilTest, end_edge) {
  typedef typename TypeParam::first_type CoordType;
  typedef typename TypeParam::second_type EdgeType;
  constexpr auto check = [](const CoordType coord,
                            const scipp::index expected) {
    const std::vector<EdgeType> edges{0, 1, 2, 3};
    scipp::index bin{0};
    scipp::index index;
    end_edge(bin, index, coord, edges);
    EXPECT_EQ(index, expected);
  };
  check(-0.1, 2);
  check(0.0, 2);
  check(1.0, 2);
  check(1.5, 3);
  check(2.0, 3);
  check(2.5, 4);
  check(3.0, 4);
}
