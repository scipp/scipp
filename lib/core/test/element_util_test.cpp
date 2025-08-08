// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#include <cmath>
#include <gtest/gtest.h>
#include <vector>

#include "scipp/core/element/util.h"
#include "scipp/core/time_point.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core::element;

TEST(ElementUtilTest, where) {
  EXPECT_EQ(where(true, 1, 2), 1);
  EXPECT_EQ(where(false, 1, 2), 2);
}

TEST(ElementUtilTest, where_unit_preserved) {
  for (const auto &unit : {sc_units::m, sc_units::one, sc_units::s}) {
    EXPECT_EQ(where(sc_units::none, unit, unit), unit);
  }
}

TEST(ElementUtilTest, where_unit_mismatch_fail) {
  for (const auto &unit : {sc_units::m, sc_units::one, sc_units::s}) {
    EXPECT_THROW(where(sc_units::none, unit, sc_units::kg), except::UnitError);
  }
}

TEST(ElementUtilTest, where_rejects_condition_with_unit) {
  EXPECT_NO_THROW_DISCARD(where(sc_units::none, sc_units::m, sc_units::m));
  for (const auto &unit :
       {sc_units::m, sc_units::kg, sc_units::s, sc_units::one}) {
    EXPECT_THROW(where(unit, sc_units::m, sc_units::m), except::UnitError);
  }
}

TEST(ElementUtilTest, where_accepts_all_types) {
  static_assert(std::is_same_v<decltype(where(true, bool{}, bool{})), bool>);
  static_assert(
      std::is_same_v<decltype(where(true, double{}, double{})), double>);
  static_assert(std::is_same_v<decltype(where(true, float{}, float{})), float>);
  static_assert(
      std::is_same_v<decltype(where(true, int32_t{}, int32_t{})), int32_t>);
  static_assert(
      std::is_same_v<decltype(where(true, int64_t{}, int64_t{})), int64_t>);
  static_assert(std::is_same_v<decltype(where(true, core::time_point{},
                                              core::time_point{})),
                               core::time_point>);
}

TEST(ElementUtilTest, values_variances_stddev) {
  ValueAndVariance x{1.0, 2.0};
  EXPECT_EQ(values(sc_units::m), sc_units::m);
  EXPECT_EQ(values(x), 1.0);
  EXPECT_EQ(values(1.2), 1.2);
  EXPECT_EQ(variances(sc_units::m), sc_units::m * sc_units::m);
  EXPECT_EQ(variances(x), 2.0);
  EXPECT_EQ(stddevs(sc_units::m), sc_units::m);
  EXPECT_EQ(stddevs(sc_units::counts), sc_units::counts);
  EXPECT_EQ(stddevs(x), sqrt(2.0));
}

namespace {
constexpr auto test_issorted = [](const auto sorted, const bool order) {
  const auto expect_sorted_eq = [&sorted](const auto a, const auto b,
                                          const auto expected) {
    bool out = true;
    sorted(out, a, b);
    EXPECT_EQ(out, expected);
  };
  expect_sorted_eq(1.0, 2.0, order);
  expect_sorted_eq(-1.0, 1.0, order);
  expect_sorted_eq(-2.0, -1.0, order);
  expect_sorted_eq(1.0, 1.0, true);
  expect_sorted_eq(2.0, 1.0, !order);
  expect_sorted_eq(1.0, -1.0, !order);
  expect_sorted_eq(-1.0, -2.0, !order);
  sc_units::Unit unit = sc_units::one;
  sorted(unit, sc_units::m, sc_units::m);
  EXPECT_EQ(unit, sc_units::none);
  EXPECT_THROW(sorted(unit, sc_units::m, sc_units::s), except::UnitError);
};
} // namespace

TEST(ElementUtilTest, issorted) {
  test_issorted(issorted_nondescending, true);
  test_issorted(issorted_nonascending, false);
}

TEST(ElementUtilTest, islinspace_time_point) {
  const auto islinspace_wrapper = [](std::initializer_list<int64_t> ints) {
    std::vector<core::time_point> vec;
    vec.reserve(ints.size());
    for (auto x : ints) {
      vec.emplace_back(x);
    }

    return core::element::islinspace(std::span<const core::time_point>(vec));
  };

  EXPECT_FALSE(islinspace_wrapper({}));
  EXPECT_FALSE(islinspace_wrapper({0}));
  EXPECT_FALSE(islinspace_wrapper({0, 1, 1}));
  EXPECT_FALSE(islinspace_wrapper({0, 0, 1}));
  EXPECT_FALSE(islinspace_wrapper({1, 1, 1}));
  EXPECT_FALSE(islinspace_wrapper({0, 1, 3}));

  EXPECT_TRUE(islinspace_wrapper({0, 1}));
  EXPECT_TRUE(islinspace_wrapper({0, 1, 2}));
  EXPECT_TRUE(islinspace_wrapper({0, 2, 4, 6}));
}

TEST(ElementUtilTest, zip) {
  EXPECT_EQ(zip(1, 2), (std::pair{1, 2}));
  EXPECT_EQ(zip(3, 4), (std::pair{3, 4}));
  EXPECT_EQ(zip(sc_units::m, sc_units::m), sc_units::m);
  EXPECT_EQ(zip(sc_units::s, sc_units::s), sc_units::s);
  EXPECT_THROW(zip(sc_units::m, sc_units::s), except::UnitError);
}

TEST(ElementUtilTest, get) {
  EXPECT_EQ(core::element::get<0>(std::pair{1, 2}), 1);
  EXPECT_EQ(core::element::get<1>(std::pair{1, 2}), 2);
  EXPECT_EQ(core::element::get<0>(std::pair{3, 4}), 3);
  EXPECT_EQ(core::element::get<1>(std::pair{3, 4}), 4);
  EXPECT_EQ(core::element::get<0>(sc_units::m), sc_units::m);
  EXPECT_EQ(core::element::get<0>(sc_units::s), sc_units::s);
  EXPECT_EQ(core::element::get<1>(sc_units::m), sc_units::m);
  EXPECT_EQ(core::element::get<1>(sc_units::s), sc_units::s);
}

TEST(ElementUtilTest, fill) {
  double f64;
  float f32;
  ValueAndVariance x{1.0, 2.0};
  sc_units::Unit u;
  fill(f64, 4.5);
  EXPECT_EQ(f64, 4.5);
  fill(f32, 4.5);
  EXPECT_EQ(f32, 4.5);
  fill(x, 4.5);
  EXPECT_EQ(x, (ValueAndVariance{4.5, 0.0}));
  fill(x, ValueAndVariance{1.2, 3.4});
  EXPECT_EQ(x, (ValueAndVariance{1.2, 3.4}));
  fill(u, sc_units::m);
  EXPECT_EQ(u, sc_units::m);
}

TEST(ElementUtilTest, fill_zeros) {
  double x = 1.2;
  ValueAndVariance y{1.0, 2.0};
  sc_units::Unit u = sc_units::m;
  fill_zeros(x);
  EXPECT_EQ(x, 0.0);
  fill_zeros(y);
  EXPECT_EQ(y, (ValueAndVariance{0.0, 0.0}));
  fill_zeros(u);
  EXPECT_EQ(u, sc_units::m); // unchanged
}
