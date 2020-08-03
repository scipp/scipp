// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/string.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::core;

TEST(CoreStringTest, convertToIsoDate_test) {
  const int64_t ts(1595846471200000011);
  scipp::core::time_point date(ts);
  const units::Unit unit(units::ns);
  EXPECT_EQ(to_iso_date(date, unit), "2020-07-27T10:41:11.200000011\n");

  const int64_t ts2(1595846471);
  scipp::core::time_point date2(ts2);
  const units::Unit unit2(units::s);
  EXPECT_EQ(to_iso_date(date2, unit2), "2020-07-27T10:41:11\n");

  const units::Unit bad_unit(units::m);
  EXPECT_THROW(to_iso_date(date2, bad_unit), scipp::except::UnitError);
  EXPECT_THROW(to_iso_date(date2), scipp::except::UnitError);
}
