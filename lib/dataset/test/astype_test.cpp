// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/astype.h"
#include "scipp/dataset/data_array.h"
#include "scipp/variable/astype.h"
#include "scipp/variable/variable.h"

using namespace scipp;

class AstypeDataArrayTest : public ::testing::Test {
protected:
  const DataArray data_array = DataArray(
      makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{4, 5, 6})}},
      {{"m", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                Values{false, true, true})}});

  static void do_check(const DataArray &original, const DType target_dtype,
                       const CopyPolicy copy_policy, const bool expect_copy) {
    const auto converted = astype(original, target_dtype, copy_policy);

    EXPECT_EQ(converted.data(), astype(original.data(), target_dtype));
    EXPECT_EQ(converted.masks(), original.masks());

    EXPECT_TRUE(converted.coords()[Dim::X].is_same(original.coords()[Dim::X]));
    EXPECT_EQ(converted.data().is_same(original.data()), !expect_copy);
    EXPECT_EQ(converted.masks()["m"].is_same(original.masks()["m"]),
              !expect_copy);
  }
};

TEST_F(AstypeDataArrayTest, different_type) {
  do_check(data_array, dtype<double>, CopyPolicy::TryAvoid, true);
  do_check(data_array, dtype<double>, CopyPolicy::Always, true);
}

TEST_F(AstypeDataArrayTest, same_type) {
  do_check(data_array, dtype<int>, CopyPolicy::TryAvoid, false);
  do_check(data_array, dtype<int>, CopyPolicy::Always, true);
}
