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
    return core::time_point{chrono::duration_cast<Duration>(t).count()};
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
  EXPECT_EQ(to_iso_date(t, units::Unit{"day"}), "2020-07-27");
}

TEST_F(ISODateTest, months) {
  units::Unit m{"month"};
  EXPECT_EQ(to_iso_date(core::time_point{0}, m), "1970-01");
  EXPECT_EQ(to_iso_date(core::time_point{1}, m), "1970-02");
  EXPECT_EQ(to_iso_date(core::time_point{12}, m), "1971-01");
  EXPECT_EQ(to_iso_date(core::time_point{15}, m), "1971-04");
  EXPECT_EQ(to_iso_date(core::time_point{-1}, m), "1969-12");
  EXPECT_EQ(to_iso_date(core::time_point{-5}, m), "1969-08");
  EXPECT_EQ(to_iso_date(core::time_point{-12}, m), "1969-01");
  EXPECT_EQ(to_iso_date(core::time_point{-18}, m), "1968-07");
}

TEST_F(ISODateTest, years) {
  units::Unit y{"year"};
  EXPECT_EQ(to_iso_date(core::time_point{0}, y), "1970");
  EXPECT_EQ(to_iso_date(core::time_point{1}, y), "1971");
  EXPECT_EQ(to_iso_date(core::time_point{13}, y), "1983");
  EXPECT_EQ(to_iso_date(core::time_point{-1}, y), "1969");
  EXPECT_EQ(to_iso_date(core::time_point{-6}, y), "1964");
}

TEST_F(ISODateTest, invalid_unit) {
  EXPECT_THROW(to_iso_date(get_time<chrono::minutes>(), units::m),
               scipp::except::UnitError);
}
