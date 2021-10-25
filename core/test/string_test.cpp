// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <chrono>
#include <sstream>

#include "scipp/core/string.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

using namespace scipp;
namespace chrono = std::chrono;

class ISODateTest : public ::testing::Test {
protected:
  template <class Duration> constexpr auto get_time() {
    constexpr chrono::nanoseconds t{1595846471123456789l};
    return scipp::core::time_point{chrono::duration_cast<Duration>(t).count()};
  }
};

TEST_F(ISODateTest, ns) {
  const auto t = get_time<chrono::nanoseconds>();
  EXPECT_EQ(to_iso_date(t, units::ns), "2020-07-27T10:41:11.123456789");
}

TEST_F(ISODateTest, us) {
  const auto t = get_time<chrono::microseconds>();
  EXPECT_EQ(to_iso_date(t, units::us), "2020-07-27T10:41:11.123456");
}

TEST_F(ISODateTest, ms) {
  const auto t = get_time<chrono::milliseconds>();
  EXPECT_EQ(to_iso_date(t, units::Unit{"ms"}), "2020-07-27T10:41:11.123");
}

TEST_F(ISODateTest, s) {
  const auto t = get_time<chrono::seconds>();
  EXPECT_EQ(to_iso_date(t, units::s), "2020-07-27T10:41:11");
}

TEST_F(ISODateTest, min) {
  const auto t = get_time<chrono::minutes>();
  EXPECT_EQ(to_iso_date(t, units::Unit{"min"}), "2020-07-27T10:41:00");
}

TEST_F(ISODateTest, h) {
  const auto t = get_time<chrono::hours>();
  EXPECT_EQ(to_iso_date(t, units::Unit{"h"}), "2020-07-27T10:00:00");
}

TEST_F(ISODateTest, days) {
  const auto t = get_time<chrono::days>();
  EXPECT_EQ(to_iso_date(t, units::Unit{"day"}), "2020-07-27T00:00:00");
}

/*
 * Months and years are mean gregorian months and years.
 * The formatted time thus contains contributions from smaller
 * units down to seconds.
 */
TEST_F(ISODateTest, months) {
  for (auto m : {0, 1, 427}) {
    const auto months = chrono::months{m};
    const auto tp_months = scipp::core::time_point{months.count()};
    const auto seconds = chrono::duration_cast<chrono::seconds>(months);
    const auto tp_seconds = scipp::core::time_point{seconds.count()};
    EXPECT_EQ(to_iso_date(tp_months, units::Unit{"month"}),
              to_iso_date(tp_seconds, units::s));
  }
}

TEST_F(ISODateTest, years) {
  for (auto y : {0, 1, 23}) {
    const auto years = chrono::years{y};
    const auto tp_years = scipp::core::time_point{years.count()};
    const auto seconds = chrono::duration_cast<chrono::seconds>(years);
    const auto tp_seconds = scipp::core::time_point{seconds.count()};
    EXPECT_EQ(to_iso_date(tp_years, units::Unit{"year"}),
              to_iso_date(tp_seconds, units::s));
  }
}

TEST_F(ISODateTest, invalid_unit) {
  EXPECT_THROW(to_iso_date(get_time<chrono::minutes>(), units::m),
               scipp::except::UnitError);
}
