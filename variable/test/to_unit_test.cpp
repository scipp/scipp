// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/to_unit.h"

using namespace scipp;

TEST(ToUnitTest, int_not_supported) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<int32_t>(dims, units::Unit("m"), Values{1, 2});
  EXPECT_THROW(to_unit(var, units::Unit("mm")), except::TypeError);
}

TEST(ToUnitTest, not_compatible) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<float>(dims, units::Unit("m"), Values{1, 2});
  EXPECT_THROW(to_unit(var, units::Unit("s")), except::UnitError);
}

TEST(ToUnitTest, same) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<float>(dims, units::Unit("m"), Values{1, 2});
  EXPECT_EQ(to_unit(var, var.unit()), var);
}

TEST(ToUnitTest, m_to_mm) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<float>(dims, units::Unit("m"), Values{1, 2});
  EXPECT_EQ(to_unit(var, units::Unit("mm")),
            makeVariable<float>(dims, units::Unit("mm"), Values{1000, 2000}));
}

TEST(ToUnitTest, mm_to_m) {
  const Dimensions dims(Dim::X, 2);
  const auto var =
      makeVariable<float>(dims, units::Unit("mm"), Values{100, 1000});
  EXPECT_EQ(to_unit(var, units::Unit("m")),
            makeVariable<float>(dims, units::Unit("m"), Values{0.1, 1.0}));
}
