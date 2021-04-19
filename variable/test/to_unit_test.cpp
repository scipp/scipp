// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/variable/bins.h"
#include "scipp/variable/string.h"
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

TEST(ToUnitTest, time_point) {
  const Dimensions dims(Dim::X, 8);
  const auto var = makeVariable<core::time_point>(
      dims, units::Unit("s"),
      Values{core::time_point{10}, core::time_point{20}, core::time_point{30},
             core::time_point{40}, core::time_point{10 + 60},
             core::time_point{20 + 60}, core::time_point{30 + 60},
             core::time_point{40 + 60}});
  EXPECT_EQ(
      to_unit(var, units::Unit("min")),
      makeVariable<core::time_point>(
          dims, units::Unit("min"),
          Values{core::time_point{0}, core::time_point{0}, core::time_point{1},
                 core::time_point{1}, core::time_point{1}, core::time_point{1},
                 core::time_point{2}, core::time_point{2}}));
  EXPECT_EQ(to_unit(var, units::Unit("ms")),
            makeVariable<core::time_point>(
                dims, units::Unit("ms"),
                Values{core::time_point{10000}, core::time_point{20000},
                       core::time_point{30000}, core::time_point{40000},
                       core::time_point{10000 + 60000},
                       core::time_point{20000 + 60000},
                       core::time_point{30000 + 60000},
                       core::time_point{40000 + 60000}}));
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

TEST(ToUnitTest, binned) {
  const auto indices = makeVariable<scipp::index_pair>(
      Dims{Dim::Y}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 4}});
  const auto input_buffer =
      makeVariable<double>(Dims{Dim::X}, Shape{4},
                           Values{1000, 2000, 3000, 4000}, units::Unit{"mm"});
  const auto expected_buffer = makeVariable<double>(
      Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4}, units::Unit{"m"});
  const auto var = make_bins(indices, Dim::X, input_buffer);
  EXPECT_EQ(to_unit(var, units::Unit{"m"}),
            make_bins(indices, Dim::X, expected_buffer));
}