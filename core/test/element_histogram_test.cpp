// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/constants.h"
#include "scipp/core/element/histogram.h"
#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::core;

TEST(ElementHistogramTest, variance_flags) {
  static_assert(std::is_base_of_v<transform_flags::expect_variance_arg_t<0>,
                                  decltype(element::histogram)>);
  static_assert(std::is_base_of_v<transform_flags::expect_no_variance_arg_t<1>,
                                  decltype(element::histogram)>);
  static_assert(std::is_base_of_v<transform_flags::expect_variance_arg_t<2>,
                                  decltype(element::histogram)>);
  static_assert(std::is_base_of_v<transform_flags::expect_no_variance_arg_t<3>,
                                  decltype(element::histogram)>);
}

TEST(ElementHistogramTest, unit) {
  // Note that this is an operator for `transform_subspan`, so the overload for
  // units has one argument fewer than the one for data.
  for (const auto unit : {units::counts, units::one})
    EXPECT_EQ(element::histogram(units::m, unit, units::m), unit);
}

TEST(ElementHistogramTest, event_and_edge_unit_must_match) {
  EXPECT_NO_THROW(element::histogram(units::m, units::counts, units::m));
  EXPECT_NO_THROW(element::histogram(units::s, units::counts, units::s));
  EXPECT_THROW(element::histogram(units::m, units::counts, units::s),
               except::UnitError);
  EXPECT_THROW(element::histogram(units::s, units::counts, units::m),
               except::UnitError);
}

TEST(ElementHistogramTest, weight_unit_must_be_counts_or_one) {
  EXPECT_NO_THROW(element::histogram(units::m, units::counts, units::m));
  EXPECT_NO_THROW(element::histogram(units::m, units::one, units::m));
  EXPECT_THROW(element::histogram(units::m, units::s, units::m),
               except::UnitError);
  EXPECT_THROW(element::histogram(units::m, units::m, units::m),
               except::UnitError);
}

TEST(ElementHistogramTest, values) {
  std::vector<double> edges{2, 4, 6};
  std::vector<double> events{1, 2, 3, 4, 5, 6, 7};
  std::vector<double> weight_vals{10, 20, 30, 40, 50, 60, 70};
  std::vector<double> weight_vars{100, 200, 300, 400, 500, 600, 700};
  std::vector<double> result_vals{0, 0};
  std::vector<double> result_vars{0, 0};
  element::histogram(ValueAndVariance(span(result_vals), span(result_vars)),
                     events, ValuesAndVariances(weight_vals, weight_vars),
                     edges);
  EXPECT_EQ(result_vals, std::vector<double>({20 + 30, 40 + 50}));
  EXPECT_EQ(result_vars, std::vector<double>({200 + 300, 400 + 500}));
}
