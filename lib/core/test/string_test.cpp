// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
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
  EXPECT_EQ(to_iso_date(t, sc_units::ns), "2020-07-27T10:41:11.123456789");
}

TEST_F(ISODateTest, us) {
  const auto t = get_time<chrono::microseconds>();
  EXPECT_EQ(to_iso_date(t, sc_units::us), "2020-07-27T10:41:11.123456");
}

TEST_F(ISODateTest, ms) {
  const auto t = get_time<chrono::milliseconds>();
  EXPECT_EQ(to_iso_date(t, sc_units::Unit{"ms"}), "2020-07-27T10:41:11.123");
}

TEST_F(ISODateTest, s) {
  const auto t = get_time<chrono::seconds>();
  EXPECT_EQ(to_iso_date(t, sc_units::s), "2020-07-27T10:41:11");
}

TEST_F(ISODateTest, min) {
  const auto t = get_time<chrono::minutes>();
  EXPECT_EQ(to_iso_date(t, sc_units::Unit{"min"}), "2020-07-27T10:41");
}

TEST_F(ISODateTest, h) {
  const auto t = get_time<chrono::hours>();
  EXPECT_EQ(to_iso_date(t, sc_units::Unit{"h"}), "2020-07-27T10");
}

TEST_F(ISODateTest, day) {
  const auto t = get_time<chrono::days>();
  EXPECT_EQ(to_iso_date(t, sc_units::Unit{"day"}), "2020-07-27");
}

TEST_F(ISODateTest, month) {
  const auto t = get_time<chrono::months>();
  EXPECT_EQ(to_iso_date(t, sc_units::Unit{"month"}), "2020-07");
}

TEST_F(ISODateTest, year) {
  const auto t = get_time<chrono::years>();
  EXPECT_EQ(to_iso_date(t, sc_units::Unit{"year"}), "2020");
}

TEST_F(ISODateTest, invalid_unit) {
  EXPECT_THROW(to_iso_date(get_time<chrono::minutes>(), sc_units::m),
               scipp::except::UnitError);
}
