// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/element/event_operations.h"
#include "scipp/core/values_and_variances.h"

#include "fix_typed_test_suite_warnings.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementEventCopyIfTest, no_variances_allowed_in_select_arg) {
  static_assert(!std::is_base_of_v<transform_flags::expect_no_variance_arg_t<0>,
                                   decltype(element::event::copy_if)>);
  static_assert(std::is_base_of_v<transform_flags::expect_no_variance_arg_t<1>,
                                  decltype(element::event::copy_if)>);
}

template <typename T> class ElementEventCopyIfTest : public ::testing::Test {};
using ElementEventCopyIfTestTypes =
    ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(ElementEventCopyIfTest, ElementEventCopyIfTestTypes);

TYPED_TEST(ElementEventCopyIfTest, values) {
  event_list<TypeParam> values{10, 20, 30, 40};
  event_list<int32_t> select{0, 2};
  EXPECT_EQ(element::event::copy_if(values, select),
            (event_list<TypeParam>{10, 30}));
}

TYPED_TEST(ElementEventCopyIfTest, values_and_variances) {
  event_list<TypeParam> values{10, 20, 30, 40};
  event_list<TypeParam> variances{11, 22, 33, 44};
  core::ValuesAndVariances data{values, variances};
  event_list<int32_t> select{0, 2};
  EXPECT_EQ(
      element::event::copy_if(data, select),
      std::pair(event_list<TypeParam>{10, 30}, event_list<TypeParam>{11, 33}));
}

TEST(ElementEventCopyIfTest, unit) {
  units::Unit m(units::m);
  units::Unit s(units::s);
  units::Unit one(units::dimensionless);
  EXPECT_EQ(element::event::copy_if(m, one), m);
  EXPECT_EQ(element::event::copy_if(s, one), s);
  EXPECT_THROW(element::event::copy_if(m, s), except::UnitError);
  EXPECT_THROW(element::event::copy_if(m, m), except::UnitError);
}

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

TEST(ElementEventMakeSelectTest, no_variances_allowed) {
  static_assert(
      std::is_base_of_v<transform_flags::expect_no_variance_arg_t<0>,
                        decltype(element::event::make_select<int32_t>)>);
  static_assert(
      std::is_base_of_v<transform_flags::expect_no_variance_arg_t<1>,
                        decltype(element::event::make_select<int32_t>)>);
}

TEST(ElementEventMakeSelectTest, unit) {
  units::Unit m(units::m);
  units::Unit s(units::s);
  EXPECT_EQ(element::event::make_select<int32_t>(m, m), units::dimensionless);
  EXPECT_EQ(element::event::make_select<int32_t>(s, s), units::dimensionless);
  EXPECT_THROW(element::event::make_select<int32_t>(m, s), except::UnitError);
  EXPECT_THROW(element::event::make_select<int32_t>(s, m), except::UnitError);
}

TEST(ElementEventMakeSelectTest, data) {
  event_list<double> events{1.1, 2.0, 3.3, 4.0, 5.5, 3.2};
  std::vector<double> interval{2, 4};
  EXPECT_EQ(element::event::make_select<int32_t>(events, interval),
            (event_list<int32_t>{1, 2, 5}));
  EXPECT_EQ(element::event::make_select<int64_t>(events, interval),
            (event_list<int64_t>{1, 2, 5}));
}
