// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/values_and_variances.h"

#include "../element_event_operations.h"
#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

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
  event_list<TypeParam> events{0, 1, 2, 3, 4, 5, 3};
  std::vector<TypeParam> edges{0, 2, 4};
  std::vector<float> weights{2, 4};
  EXPECT_EQ(element::event::map(events, edges, weights),
            (event_list<float>{2, 2, 4, 4, 0, 0, 4}));
}

TYPED_TEST(ElementEventMapTest, variable_bin_width) {
  event_list<TypeParam> events{0, 1, 2, 3, 4, 5, 3};
  std::vector<TypeParam> edges{1, 2, 4};
  std::vector<float> weights{2, 4};
  EXPECT_EQ(element::event::map(events, edges, weights),
            (event_list<float>{0, 2, 4, 4, 0, 0, 4}));
}

TYPED_TEST(ElementEventMapTest, edges_not_sorted) {
  event_list<TypeParam> events{0, 1, 2, 3, 4, 5, 3};
  std::vector<TypeParam> edges{1, 4, 2};
  std::vector<float> weights{2, 4};
  EXPECT_THROW(element::event::map(events, edges, weights),
               except::BinEdgeError);
}

TYPED_TEST(ElementEventMapTest, variances_constant_bin_width) {
  event_list<TypeParam> events{0, 1, 2, 3, 4, 5, 3};
  std::vector<TypeParam> edges{0, 2, 4};
  std::vector<float> values{2, 4};
  std::vector<float> variances{3, 5};
  ValueAndVariance weights{scipp::span<const float>(values),
                           scipp::span<const float>(variances)};
  EXPECT_EQ(element::event::map(events, edges, weights),
            std::pair(event_list<float>{2, 2, 4, 4, 0, 0, 4},
                      event_list<float>{3, 3, 5, 5, 0, 0, 5}));
}

TYPED_TEST(ElementEventMapTest, variances_variable_bin_width) {
  event_list<TypeParam> events{0, 1, 2, 3, 4, 5, 3};
  std::vector<TypeParam> edges{1, 2, 4};
  std::vector<float> values{2, 4};
  std::vector<float> variances{3, 5};
  ValueAndVariance weights{scipp::span<const float>(values),
                           scipp::span<const float>(variances)};
  EXPECT_EQ(element::event::map(events, edges, weights),
            std::pair(event_list<float>{0, 2, 4, 4, 0, 0, 4},
                      event_list<float>{0, 3, 5, 5, 0, 0, 5}));
}
