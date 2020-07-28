// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/string.h"
#include "scipp/units/except.h"

using namespace scipp;
using namespace scipp::core;

TEST(CoreStringTest, convertToIsoDate_test) {
  int64_t ts(1595846471200000011);
  scipp::core::time_point date(ts);
  std::string unit("ns");
  EXPECT_EQ(to_iso_date(date, unit), "2020-07-27T10:41:11.200000011\n");

  int64_t ts2(1595846471);
  scipp::core::time_point date2(ts2);
  std::string unit2("s");
  EXPECT_EQ(to_iso_date(date2, unit2), "2020-07-27T10:41:11\n");

  std::string bad_unit("foo");
  EXPECT_THROW(to_iso_date(date2, bad_unit), scipp::except::UnitError);
}
