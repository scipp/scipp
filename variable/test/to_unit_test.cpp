// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/to_unit.h"
#include "test_macros.h"

using namespace scipp;

TEST(ToUnitTest, not_compatible) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<float>(dims, units::Unit("m"), Values{1, 2});
  EXPECT_THROW_DISCARD(to_unit(var, units::Unit("s")), except::UnitError);
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

TEST(ToUnitTest, ints) {
  const Dimensions dims(Dim::X, 2);
  const auto var =
      makeVariable<int32_t>(dims, units::Unit("mm"), Values{100, 2000});
  EXPECT_EQ(to_unit(var, units::Unit("m")),
            makeVariable<int32_t>(dims, units::Unit("m"), Values{0, 2}));
  EXPECT_EQ(
      to_unit(var, units::Unit("um")),
      makeVariable<int32_t>(dims, units::Unit("um"), Values{100000, 2000000}));
}

TEST(ToUnitTest, time_point_bad_units) {
  const auto do_to_unit = [](const char *initial, const char *final) {
    return to_unit(makeVariable<core::time_point>(Dims{}, units::Unit(initial)),
                   units::Unit(final));
  };

  // Conversions to or from time points with unit day or larger are complicated
  // and not implemented.
  const auto small_unit_names = {"h", "min", "s", "ns"};
  const auto large_unit_names = {"Y", "M", "D"};
  for (const char *initial : small_unit_names) {
    for (const char *final : small_unit_names) {
      EXPECT_NO_THROW_DISCARD(do_to_unit(initial, final));
    }
    for (const char *final : large_unit_names) {
      EXPECT_THROW_DISCARD(do_to_unit(initial, final), except::UnitError);
    }
  }
  for (const char *initial : large_unit_names) {
    for (const char *final : small_unit_names) {
      EXPECT_THROW_DISCARD(do_to_unit(initial, final), except::UnitError);
    }
    for (const char *final : large_unit_names) {
      EXPECT_THROW_DISCARD(do_to_unit(initial, final), except::UnitError);
    }
  }
}