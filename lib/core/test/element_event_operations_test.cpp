// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/event_operations.h"
#include "scipp/core/values_and_variances.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

using element::event::lookup_previous;
using element::event::map_linspace;
using element::event::map_sorted_edges;

TEST(ElementEventMapTest, unit) {
  sc_units::Unit kg(sc_units::kg);
  sc_units::Unit m(sc_units::m);
  sc_units::Unit s(sc_units::s);
  EXPECT_EQ(element::event::map(m, m, s, s), s);
  EXPECT_EQ(element::event::map(m, m, kg, kg), kg);
  EXPECT_THROW(element::event::map(m, s, s, s), except::UnitError);
  EXPECT_THROW(element::event::map(s, m, s, s), except::UnitError);
  EXPECT_THROW(element::event::map(m, s, kg, kg), except::UnitError);
  EXPECT_THROW(element::event::map(s, m, kg, kg), except::UnitError);
}

TEST(ElementEventMapTest, fill_unit_must_match_weight_unit) {
  sc_units::Unit kg(sc_units::kg);
  sc_units::Unit m(sc_units::m);
  sc_units::Unit s(sc_units::s);
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
  ValueAndVariance weights{std::span<const float>(values),
                           std::span<const float>(variances)};
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
  ValueAndVariance weights{std::span<const float>(values),
                           std::span<const float>(variances)};
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

class ElementLookupPreviousTest : public ::testing::Test {
protected:
  std::vector<double> x{0, 2, 4};
  std::vector<double> weights{11, 22, 33};
  double fill = 66;
};

TEST_F(ElementLookupPreviousTest, below_gives_fill_value) {
  fill = 66;
  EXPECT_EQ(lookup_previous(-0.1, x, weights, fill), 66);
}

TEST_F(ElementLookupPreviousTest, at_lowest_gives_lowest) {
  EXPECT_EQ(lookup_previous(0, x, weights, fill), 11);
}

TEST_F(ElementLookupPreviousTest, below_second_gives_lowest) {
  EXPECT_EQ(lookup_previous(1, x, weights, fill), 11);
}

TEST_F(ElementLookupPreviousTest, at_second_gives_second) {
  EXPECT_EQ(lookup_previous(2, x, weights, fill), 22);
}

TEST_F(ElementLookupPreviousTest, below_third_gives_second) {
  EXPECT_EQ(lookup_previous(3, x, weights, fill), 22);
}

TEST_F(ElementLookupPreviousTest, large_value_gives_last) {
  EXPECT_EQ(lookup_previous(123456789, x, weights, fill), 33);
}
