// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
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
  EXPECT_EQ(element::event::map(m, m, s, s), s);
  EXPECT_EQ(element::event::map(m, m, kg, kg), kg);
  EXPECT_THROW(element::event::map(m, s, s, s), except::UnitError);
  EXPECT_THROW(element::event::map(s, m, s, s), except::UnitError);
  EXPECT_THROW(element::event::map(m, s, kg, kg), except::UnitError);
  EXPECT_THROW(element::event::map(s, m, kg, kg), except::UnitError);
}

TEST(ElementEventMapTest, fill_unit_must_match_weight_unit) {
  units::Unit kg(units::kg);
  units::Unit m(units::m);
  units::Unit s(units::s);
  EXPECT_EQ(element::event::map(m, m, kg, kg), kg);
  EXPECT_THROW(element::event::map(m, m, kg, s), except::UnitError);
}

template <typename T> class ElementEventMapTest : public ::testing::Test {};
using ElementEventMapTestTypes =
    ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(ElementEventMapTest, ElementEventMapTestTypes);

TYPED_TEST(ElementEventMapTest, constant_bin_width) {
  std::vector<TypeParam> edges{0, 2, 4};
  std::vector<float> weights{2, 4};
  float fill = 66;
  EXPECT_EQ(map_linspace(TypeParam{0}, edges, weights, fill), float{2});
  EXPECT_EQ(map_linspace(TypeParam{1}, edges, weights, fill), float{2});
  EXPECT_EQ(map_linspace(TypeParam{2}, edges, weights, fill), float{4});
  EXPECT_EQ(map_linspace(TypeParam{3}, edges, weights, fill), float{4});
  EXPECT_EQ(map_linspace(TypeParam{4}, edges, weights, fill), float{66});
  EXPECT_EQ(map_linspace(TypeParam{5}, edges, weights, fill), float{66});
}

TYPED_TEST(ElementEventMapTest, variable_bin_width) {
  std::vector<TypeParam> edges{1, 2, 4};
  std::vector<float> weights{2, 4};
  float fill = 66;
  EXPECT_EQ(map_sorted_edges(TypeParam{0}, edges, weights, fill), float{66});
  EXPECT_EQ(map_sorted_edges(TypeParam{1}, edges, weights, fill), float{2});
  EXPECT_EQ(map_sorted_edges(TypeParam{2}, edges, weights, fill), float{4});
  EXPECT_EQ(map_sorted_edges(TypeParam{3}, edges, weights, fill), float{4});
  EXPECT_EQ(map_sorted_edges(TypeParam{4}, edges, weights, fill), float{66});
  EXPECT_EQ(map_sorted_edges(TypeParam{5}, edges, weights, fill), float{66});
}

TYPED_TEST(ElementEventMapTest, variances_constant_bin_width) {
  std::vector<TypeParam> edges{0, 2, 4};
  std::vector<float> values{2, 4};
  std::vector<float> variances{3, 5};
  ValueAndVariance weights{scipp::span<const float>(values),
                           scipp::span<const float>(variances)};
  ValueAndVariance<float> fill(66, 0);
  EXPECT_EQ(map_linspace(TypeParam{0}, edges, weights, fill),
            ValueAndVariance<float>(2, 3));
  EXPECT_EQ(map_linspace(TypeParam{1}, edges, weights, fill),
            ValueAndVariance<float>(2, 3));
  EXPECT_EQ(map_linspace(TypeParam{2}, edges, weights, fill),
            ValueAndVariance<float>(4, 5));
  EXPECT_EQ(map_linspace(TypeParam{3}, edges, weights, fill),
            ValueAndVariance<float>(4, 5));
  EXPECT_EQ(map_linspace(TypeParam{4}, edges, weights, fill),
            ValueAndVariance<float>(66, 0));
  EXPECT_EQ(map_linspace(TypeParam{5}, edges, weights, fill),
            ValueAndVariance<float>(66, 0));
}

TYPED_TEST(ElementEventMapTest, variances_variable_bin_width) {
  std::vector<TypeParam> edges{1, 2, 4};
  std::vector<float> values{2, 4};
  std::vector<float> variances{3, 5};
  ValueAndVariance weights{scipp::span<const float>(values),
                           scipp::span<const float>(variances)};
  ValueAndVariance<float> fill(66, 0);
  EXPECT_EQ(map_sorted_edges(TypeParam{0}, edges, weights, fill),
            ValueAndVariance<float>(66, 0));
  EXPECT_EQ(map_sorted_edges(TypeParam{1}, edges, weights, fill),
            ValueAndVariance<float>(2, 3));
  EXPECT_EQ(map_sorted_edges(TypeParam{2}, edges, weights, fill),
            ValueAndVariance<float>(4, 5));
  EXPECT_EQ(map_sorted_edges(TypeParam{3}, edges, weights, fill),
            ValueAndVariance<float>(4, 5));
  EXPECT_EQ(map_sorted_edges(TypeParam{4}, edges, weights, fill),
            ValueAndVariance<float>(66, 0));
  EXPECT_EQ(map_sorted_edges(TypeParam{5}, edges, weights, fill),
            ValueAndVariance<float>(66, 0));
}
