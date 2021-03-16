// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/event_operations.h"
#include "scipp/core/values_and_variances.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

using element::event::map_linspace;
using element::event::map_sorted_edges;

TEST(ElementEventMapTest, unit) {
  units::Unit kg(units::kg);
  units::Unit m(units::m);
  units::Unit s(units::s);
  EXPECT_EQ(element::event::map(m, m, s), s);
  EXPECT_EQ(element::event::map(m, m, kg), kg);
  EXPECT_THROW(element::event::map(m, s, s), except::UnitError);
  EXPECT_THROW(element::event::map(s, m, s), except::UnitError);
  EXPECT_THROW(element::event::map(m, s, kg), except::UnitError);
  EXPECT_THROW(element::event::map(s, m, kg), except::UnitError);
}

template <typename T> class ElementEventMapTest : public ::testing::Test {};
using ElementEventMapTestTypes =
    ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(ElementEventMapTest, ElementEventMapTestTypes);

TYPED_TEST(ElementEventMapTest, constant_bin_width) {
  std::vector<TypeParam> edges{0, 2, 4};
  std::vector<float> weights{2, 4};
  EXPECT_EQ(map_linspace(TypeParam{0}, edges, weights), float{2});
  EXPECT_EQ(map_linspace(TypeParam{1}, edges, weights), float{2});
  EXPECT_EQ(map_linspace(TypeParam{2}, edges, weights), float{4});
  EXPECT_EQ(map_linspace(TypeParam{3}, edges, weights), float{4});
  EXPECT_EQ(map_linspace(TypeParam{4}, edges, weights), float{0});
  EXPECT_EQ(map_linspace(TypeParam{5}, edges, weights), float{0});
}

TYPED_TEST(ElementEventMapTest, variable_bin_width) {
  std::vector<TypeParam> edges{1, 2, 4};
  std::vector<float> weights{2, 4};
  EXPECT_EQ(map_sorted_edges(TypeParam{0}, edges, weights), float{0});
  EXPECT_EQ(map_sorted_edges(TypeParam{1}, edges, weights), float{2});
  EXPECT_EQ(map_sorted_edges(TypeParam{2}, edges, weights), float{4});
  EXPECT_EQ(map_sorted_edges(TypeParam{3}, edges, weights), float{4});
  EXPECT_EQ(map_sorted_edges(TypeParam{4}, edges, weights), float{0});
  EXPECT_EQ(map_sorted_edges(TypeParam{5}, edges, weights), float{0});
}

TYPED_TEST(ElementEventMapTest, variances_constant_bin_width) {
  std::vector<TypeParam> edges{0, 2, 4};
  std::vector<float> values{2, 4};
  std::vector<float> variances{3, 5};
  ValueAndVariance weights{scipp::span<const float>(values),
                           scipp::span<const float>(variances)};
  EXPECT_EQ(map_linspace(TypeParam{0}, edges, weights),
            ValueAndVariance<float>(2, 3));
  EXPECT_EQ(map_linspace(TypeParam{1}, edges, weights),
            ValueAndVariance<float>(2, 3));
  EXPECT_EQ(map_linspace(TypeParam{2}, edges, weights),
            ValueAndVariance<float>(4, 5));
  EXPECT_EQ(map_linspace(TypeParam{3}, edges, weights),
            ValueAndVariance<float>(4, 5));
  EXPECT_EQ(map_linspace(TypeParam{4}, edges, weights),
            ValueAndVariance<float>(0, 0));
  EXPECT_EQ(map_linspace(TypeParam{5}, edges, weights),
            ValueAndVariance<float>(0, 0));
}

TYPED_TEST(ElementEventMapTest, variances_variable_bin_width) {
  std::vector<TypeParam> edges{1, 2, 4};
  std::vector<float> values{2, 4};
  std::vector<float> variances{3, 5};
  ValueAndVariance weights{scipp::span<const float>(values),
                           scipp::span<const float>(variances)};
  EXPECT_EQ(map_sorted_edges(TypeParam{0}, edges, weights),
            ValueAndVariance<float>(0, 0));
  EXPECT_EQ(map_sorted_edges(TypeParam{1}, edges, weights),
            ValueAndVariance<float>(2, 3));
  EXPECT_EQ(map_sorted_edges(TypeParam{2}, edges, weights),
            ValueAndVariance<float>(4, 5));
  EXPECT_EQ(map_sorted_edges(TypeParam{3}, edges, weights),
            ValueAndVariance<float>(4, 5));
  EXPECT_EQ(map_sorted_edges(TypeParam{4}, edges, weights),
            ValueAndVariance<float>(0, 0));
  EXPECT_EQ(map_sorted_edges(TypeParam{5}, edges, weights),
            ValueAndVariance<float>(0, 0));
}
