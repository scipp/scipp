// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/string.h"
#include "scipp/units/except.h"
#include "scipp/units/unit.h"

using namespace scipp;
using namespace scipp::core;

TEST(CoreStringTest, convertToIsoDate) {
  const int64_t ts(1595846471200000011);
  scipp::core::time_point date(ts);
  EXPECT_EQ(to_iso_date(date, units::ns), "2020-07-27T10:41:11.200000011");

  const int64_t ts2(1595846471);
  scipp::core::time_point date2(ts2);
  EXPECT_EQ(to_iso_date(date2, units::s), "2020-07-27T10:41:11");

  EXPECT_THROW(to_iso_date(date2, units::m), scipp::except::UnitError);
}
