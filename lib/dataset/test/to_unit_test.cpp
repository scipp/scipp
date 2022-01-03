// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/data_array.h"
#include "scipp/dataset/to_unit.h"
#include "scipp/variable/to_unit.h"

using namespace scipp;

class ToUnitDataArrayTest : public ::testing::Test {
protected:
  const DataArray data_array =
      DataArray(makeVariable<double>(Dims{Dim::X}, Shape{3},
                                     Values{1.0, 2.0, 3.0}, units::m),
                {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3},
                                            Values{4, 5, 6}, units::s)}},
                {{"m", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                          Values{true, false, true})}});

  static void do_check(const DataArray &original, const units::Unit target_unit,
                       const CopyPolicy copy_policy, const bool expect_copy) {
    const auto converted = to_unit(original, target_unit, copy_policy);

    EXPECT_EQ(converted.data(), to_unit(original.data(), target_unit));
    EXPECT_EQ(converted.coords()[Dim::X].unit(), units::s);
    EXPECT_EQ(converted.masks(), original.masks());

    EXPECT_TRUE(converted.coords()[Dim::X].is_same(original.coords()[Dim::X]));
    EXPECT_EQ(converted.data().is_same(original.data()), !expect_copy);
    EXPECT_EQ(converted.masks()["m"].is_same(original.masks()["m"]),
              !expect_copy);
  }
};

TEST_F(ToUnitDataArrayTest, different_unit) {
  do_check(data_array, units::mm, CopyPolicy::TryAvoid, true);
  do_check(data_array, units::mm, CopyPolicy::Always, true);
}

TEST_F(ToUnitDataArrayTest, same_unit) {
  do_check(data_array, units::m, CopyPolicy::TryAvoid, false);
  do_check(data_array, units::m, CopyPolicy::Always, true);
}
