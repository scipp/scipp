// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/spatial_transforms.h"
#include "scipp/variable/bins.h"
#include "scipp/variable/comparison.h"
#include "scipp/variable/reduction.h"
#include "scipp/variable/string.h"
#include "scipp/variable/to_unit.h"
#include "test_macros.h"

using namespace scipp;
using Translation = scipp::core::Translation;
using Quaternion = scipp::core::Quaternion;
using Affine3d = Eigen::Affine3d;
using Vector3d = Eigen::Vector3d;

TEST(ToUnitTest, not_compatible) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<float>(dims, sc_units::Unit("m"), Values{1, 2});
  EXPECT_THROW_DISCARD(to_unit(var, sc_units::Unit("s")), except::UnitError);
}

TEST(ToUnitTest, buffer_handling) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<float>(dims, sc_units::Unit("m"), Values{1, 2});
  const auto force_copy = to_unit(var, var.unit());
  EXPECT_FALSE(force_copy.is_same(var));
  EXPECT_EQ(force_copy.values<float>(), var.values<float>());
  const auto force_copy_explicit = to_unit(var, var.unit(), CopyPolicy::Always);
  EXPECT_FALSE(force_copy_explicit.is_same(var));
  EXPECT_EQ(force_copy_explicit.values<float>(), var.values<float>());
  const auto no_copy = to_unit(var, var.unit(), CopyPolicy::TryAvoid);
  EXPECT_TRUE(no_copy.is_same(var));
  EXPECT_EQ(no_copy.values<float>(), var.values<float>());
  const auto required_copy =
      to_unit(var, sc_units::Unit("mm"), CopyPolicy::TryAvoid);
  EXPECT_FALSE(required_copy.is_same(var));
}

TEST(ToUnitTest, same) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<float>(dims, sc_units::Unit("m"), Values{1, 2});
  EXPECT_EQ(to_unit(var, var.unit()), var);
}

TEST(ToUnitTest, m_to_mm) {
  const Dimensions dims(Dim::X, 2);
  const auto var = makeVariable<float>(dims, sc_units::Unit("m"), Values{1, 2});
  EXPECT_EQ(
      to_unit(var, sc_units::Unit("mm")),
      makeVariable<float>(dims, sc_units::Unit("mm"), Values{1000, 2000}));
}

TEST(ToUnitTest, mm_to_m) {
  const Dimensions dims(Dim::X, 2);
  const auto var =
      makeVariable<float>(dims, sc_units::Unit("mm"), Values{100, 1000});
  EXPECT_EQ(to_unit(var, sc_units::Unit("m")),
            makeVariable<float>(dims, sc_units::Unit("m"), Values{0.1, 1.0}));
}

TEST(ToUnitTest, ints) {
  const Dimensions dims(Dim::X, 2);
  const auto var =
      makeVariable<int32_t>(dims, sc_units::Unit("mm"), Values{100, 2000});
  EXPECT_EQ(to_unit(var, sc_units::Unit("m")),
            makeVariable<int32_t>(dims, sc_units::Unit("m"), Values{0, 2}));
  EXPECT_EQ(to_unit(var, sc_units::Unit("um")),
            makeVariable<int32_t>(dims, sc_units::Unit("um"),
                                  Values{100000, 2000000}));
}

TEST(ToUnitTest, time_point) {
  const Dimensions dims(Dim::X, 8);
  const auto var = makeVariable<core::time_point>(
      dims, sc_units::Unit("s"),
      Values{core::time_point{10}, core::time_point{20}, core::time_point{30},
             core::time_point{40}, core::time_point{10 + 60},
             core::time_point{20 + 60}, core::time_point{30 + 60},
             core::time_point{40 + 60}});
  EXPECT_EQ(
      to_unit(var, sc_units::Unit("min")),
      makeVariable<core::time_point>(
          dims, sc_units::Unit("min"),
          Values{core::time_point{0}, core::time_point{0}, core::time_point{1},
                 core::time_point{1}, core::time_point{1}, core::time_point{1},
                 core::time_point{2}, core::time_point{2}}));
  EXPECT_EQ(to_unit(var, sc_units::Unit("ms")),
            makeVariable<core::time_point>(
                dims, sc_units::Unit("ms"),
                Values{core::time_point{10000}, core::time_point{20000},
                       core::time_point{30000}, core::time_point{40000},
                       core::time_point{10000 + 60000},
                       core::time_point{20000 + 60000},
                       core::time_point{30000 + 60000},
                       core::time_point{40000 + 60000}}));
}

TEST(ToUnitTest, time_point_large_units) {
  const auto do_to_unit = [](const char *initial, const char *target) {
    return to_unit(
        makeVariable<core::time_point>(Dims{}, sc_units::Unit(initial)),
        sc_units::Unit(target));
  };

  // Conversions to or from time points with unit day or larger are complicated
  // and not implemented.
  const auto small_unit_names = {"h", "min", "s", "ns"};
  const auto large_unit_names = {"Y", "M", "D"};
  for (const char *initial : small_unit_names) {
    for (const char *target : small_unit_names) {
      EXPECT_NO_THROW_DISCARD(do_to_unit(initial, target));
    }
    for (const char *target : large_unit_names) {
      EXPECT_THROW_DISCARD(do_to_unit(initial, target), except::UnitError);
    }
  }
  for (const char *initial : large_unit_names) {
    for (const char *target : small_unit_names) {
      EXPECT_THROW_DISCARD(do_to_unit(initial, target), except::UnitError);
    }
    for (const char *target : large_unit_names) {
      if (initial == target)
        EXPECT_NO_THROW_DISCARD(do_to_unit(initial, target));
      else
        EXPECT_THROW_DISCARD(do_to_unit(initial, target), except::UnitError);
    }
  }
}

TEST(ToUnitTest, time_point_bad_unit) {
  EXPECT_THROW_DISCARD(
      to_unit(makeVariable<core::time_point>(Dims{}, sc_units::Unit("m")),
              sc_units::Unit("mm")),
      except::UnitError);
}

TEST(ToUnitTest, vector3d) {
  const Dimensions dims(Dim::X, 1);
  const auto var =
      makeVariable<Vector3d>(dims, Values{Vector3d{0, 1, 2}}, sc_units::m);
  const auto expected = makeVariable<Vector3d>(
      dims, Values{Vector3d{0, 1000, 2000}}, sc_units::mm);
  EXPECT_EQ(to_unit(var, sc_units::mm), expected);
}

TEST(ToUnitTest, affine3d) {
  const Eigen::AngleAxisd rotation(31.45, Eigen::Vector3d{0, 1, 0});
  const Eigen::Translation3d translation(-4, 1, 3);
  const Eigen::Affine3d affine = rotation * translation;

  const Eigen::Translation3d expected_translation(-4000, 1000, 3000);
  const Eigen::Affine3d expected_affine = rotation * expected_translation;

  const Dimensions dims(Dim::X, 1);
  const auto var = makeVariable<Affine3d>(dims, Values{affine}, sc_units::m);
  const auto expected =
      makeVariable<Affine3d>(dims, Values{expected_affine}, sc_units::mm);
  EXPECT_TRUE(all(isclose(to_unit(var, sc_units::mm), expected,
                          1e-8 * sc_units::one, 0.0 * sc_units::mm))
                  .value<bool>());
}

TEST(ToUnitTest, translation) {
  const Dimensions dims(Dim::X, 1);
  const auto var = makeVariable<Translation>(
      dims, Values{Translation{Vector3d{1, 2, 3}}}, sc_units::m);
  const auto expected = makeVariable<Translation>(
      dims, Values{Translation{Vector3d{1000, 2000, 3000}}}, sc_units::mm);
  EXPECT_EQ(to_unit(var, sc_units::mm), expected);
}

TEST(ToUnitTest, quaternion) {
  const Dimensions dims(Dim::X, 1);
  const auto var = makeVariable<Quaternion>(
      dims, Values{Quaternion{Eigen::Quaterniond{0, 0, 0, 0}}}, sc_units::m);
  EXPECT_THROW_DISCARD(to_unit(var, sc_units::mm), except::TypeError);
}

TEST(ToUnitTest, binned) {
  const auto indices = makeVariable<scipp::index_pair>(
      Dims{Dim::Y}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 4}});
  const auto input_buffer = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                                 Values{1000, 2000, 3000, 4000},
                                                 sc_units::Unit{"mm"});
  const auto expected_buffer = to_unit(input_buffer, sc_units::Unit("m"));
  const auto var = make_bins(indices, Dim::X, input_buffer);
  EXPECT_EQ(to_unit(var, sc_units::Unit{"m"}),
            make_bins(indices, Dim::X, expected_buffer));
}

TEST(ToUnitTest, binned_can_avoid_copy) {
  const auto indices = makeVariable<scipp::index_pair>(
      Dims{Dim::Y}, Shape{2}, Values{std::pair{0, 2}, std::pair{2, 4}});
  const auto input_buffer = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                                 Values{1000, 2000, 3000, 4000},
                                                 sc_units::Unit{"mm"});
  const auto var = make_bins(indices, Dim::X, input_buffer);
  EXPECT_TRUE(
      to_unit(var, sc_units::Unit{"mm"}, CopyPolicy::TryAvoid).is_same(var));
  EXPECT_FALSE(
      to_unit(var, sc_units::Unit{"mm"}, CopyPolicy::Always).is_same(var));
}

TEST(ToUnitTest, throws_if_none_unit) {
  EXPECT_THROW_DISCARD(
      to_unit(makeVariable<int32_t>(Dims{Dim::X}, Shape{2}, sc_units::none,
                                    Values{1, 2}),
              sc_units::m),
      except::UnitError);
  EXPECT_THROW_DISCARD(to_unit(makeVariable<int32_t>(Dims{Dim::X}, Shape{2},
                                                     sc_units::m, Values{1, 2}),
                               sc_units::none),
                       except::UnitError);
}

TEST(ToUnitTest, does_not_throws_if_both_are_none) {
  EXPECT_NO_THROW_DISCARD(
      to_unit(makeVariable<int32_t>(Dims{Dim::X}, Shape{2}, sc_units::none,
                                    Values{1, 2}),
              sc_units::none));
}

TEST(ToUnitTest, large_to_small_rounding_error_float) {
  const auto one_m = makeVariable<float>(sc_units::Unit("m"), Values{1});
  EXPECT_EQ(to_unit(one_m, sc_units::Unit("nm")),
            makeVariable<float>(sc_units::Unit("nm"), Values{1e9}));
  EXPECT_EQ(to_unit(one_m, sc_units::Unit("pm")),
            makeVariable<float>(sc_units::Unit("pm"), Values{1e12}));
  EXPECT_EQ(to_unit(one_m, sc_units::Unit("fm")),
            makeVariable<float>(sc_units::Unit("fm"), Values{1e15}));
  EXPECT_EQ(to_unit(one_m, sc_units::Unit("am")),
            makeVariable<float>(sc_units::Unit("am"), Values{1e18}));
}

TEST(ToUnitTest, large_to_small_rounding_error_double) {
  const auto one_m = makeVariable<double>(sc_units::Unit("m"), Values{1});
  EXPECT_EQ(to_unit(one_m, sc_units::Unit("nm")),
            makeVariable<double>(sc_units::Unit("nm"), Values{1e9}));
  EXPECT_EQ(to_unit(one_m, sc_units::Unit("pm")),
            makeVariable<double>(sc_units::Unit("pm"), Values{1e12}));
  EXPECT_EQ(to_unit(one_m, sc_units::Unit("fm")),
            makeVariable<double>(sc_units::Unit("fm"), Values{1e15}));
  EXPECT_EQ(to_unit(one_m, sc_units::Unit("am")),
            makeVariable<double>(sc_units::Unit("am"), Values{1e18}));
}

TEST(ToUnitTest, small_to_large_rounding_error_float) {
  const auto one_m = makeVariable<float>(sc_units::Unit("m"), Values{1});
  EXPECT_EQ(to_unit(makeVariable<float>(sc_units::Unit("nm"), Values{1e9}),
                    sc_units::Unit("m")),
            one_m);
  EXPECT_EQ(to_unit(makeVariable<float>(sc_units::Unit("pm"), Values{1e12}),
                    sc_units::Unit("m")),
            one_m);
  EXPECT_EQ(to_unit(makeVariable<float>(sc_units::Unit("fm"), Values{1e15}),
                    sc_units::Unit("m")),
            one_m);
  EXPECT_EQ(to_unit(makeVariable<float>(sc_units::Unit("am"), Values{1e18}),
                    sc_units::Unit("m")),
            one_m);
}

TEST(ToUnitTest, small_to_large_rounding_error_double) {
  const auto one_m = makeVariable<double>(sc_units::Unit("m"), Values{1});
  EXPECT_EQ(to_unit(makeVariable<double>(sc_units::Unit("nm"), Values{1e9}),
                    sc_units::Unit("m")),
            one_m);
  EXPECT_EQ(to_unit(makeVariable<double>(sc_units::Unit("pm"), Values{1e12}),
                    sc_units::Unit("m")),
            one_m);
  EXPECT_EQ(to_unit(makeVariable<double>(sc_units::Unit("fm"), Values{1e15}),
                    sc_units::Unit("m")),
            one_m);
  EXPECT_EQ(to_unit(makeVariable<double>(sc_units::Unit("am"), Values{1e18}),
                    sc_units::Unit("m")),
            one_m);
}

TEST(ToUnitTest, small_number_to_small_unit) {
  const auto unit = sc_units::angstrom * sc_units::angstrom;
  const auto small =
      makeVariable<double>(sc_units::Unit("m**2"), Values{1e-20});
  const auto result = to_unit(small, unit);
  EXPECT_EQ(result.unit(), unit);
  EXPECT_DOUBLE_EQ(result.value<double>(), 1.0);
}

TEST(ToUnitTest, small_number_to_small_unit_non_power_of_10) {
  const auto unit = sc_units::Unit("1.45e-21");
  const auto small = makeVariable<double>(sc_units::one, Values{1.45e-21});
  const auto result = to_unit(small, unit);
  EXPECT_EQ(result.unit(), unit);
  EXPECT_DOUBLE_EQ(result.value<double>(), 1.0);
}
