// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "dataset_test_common.h"
#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

TEST(DatasetOperationsTest, sum) {
  auto ds = make_1_values_and_variances<float>(
      "a", {Dim::X, 3}, units::dimensionless, {1, 2, 3}, {12, 15, 18});
  EXPECT_EQ(core::sum(ds, Dim::X)["a"].data(), makeVariable<float>(6, 45));
  EXPECT_EQ(core::sum(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(3, 27));
  EXPECT_THROW(core::sum(make_sparse_2d({1, 2, 3, 4}, {0, 0}), Dim::X),
               except::DimensionError);
}

TEST(DatasetOperationsTest, mean) {
  auto ds = make_1_values_and_variances<float>(
      "a", {Dim::X, 3}, units::dimensionless, {1, 2, 3}, {12, 15, 18});
  EXPECT_EQ(core::mean(ds, Dim::X)["a"].data(), makeVariable<float>(2, 5.0));
  EXPECT_EQ(core::mean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(1.5, 6.75));
}

template <typename T>
class DatasetShapeChangingOpTest : public ::testing::Test {
public:
  void SetUp() {
    ds.setData("data_x", makeVariable<T>({Dim::X, 5}, {1, 5, 4, 5, 1}));
    ds.setMask("masks_x", makeVariable<bool>(
                              {Dim::X, 5}, {false, true, false, true, false}));
  }
  Dataset ds;
};

using DataTypes = ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(DatasetShapeChangingOpTest, DataTypes);

TYPED_TEST(DatasetShapeChangingOpTest, sum_masked) {
  const auto result = sum(this->ds, Dim::X);

  ASSERT_EQ(result["data_x"].data(), makeVariable<TypeParam>({6}));
}

TYPED_TEST(DatasetShapeChangingOpTest, mean_masked) {
  const auto result = mean(this->ds, Dim::X);

  if constexpr (std::is_floating_point_v<TypeParam>)
    ASSERT_EQ(result["data_x"].data(), makeVariable<TypeParam>({2}));
  else // non floating point gets the result as a double
    ASSERT_EQ(result["data_x"].data(), makeVariable<double>({2}));
}
