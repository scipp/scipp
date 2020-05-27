// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "dataset_test_common.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/reduction.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(DatasetOperationsTest, sum) {
  auto ds = make_1_values_and_variances<float>(
      "a", {Dim::X, 3}, units::dimensionless, {1, 2, 3}, {12, 15, 18});
  EXPECT_EQ(dataset::sum(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{6}, Variances{45}));
  EXPECT_EQ(dataset::sum(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{3}, Variances{27}));
  EXPECT_THROW(dataset::sum(make_events_2d({1, 2, 3, 4}, {0, 0}), Dim::X),
               except::TypeError);
}

TEST(DatasetOperationsTest, mean) {
  auto ds = make_1_values_and_variances<float>(
      "a", {Dim::X, 3}, units::dimensionless, {1, 2, 3}, {12, 15, 18});
  EXPECT_EQ(dataset::mean(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{2}, Variances{5.0}));
  EXPECT_EQ(dataset::mean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{1.5}, Variances{6.75}));
}

template <typename T>
class DatasetShapeChangingOpTest : public ::testing::Test {
public:
  void SetUp() {
    ds.setData("data_x",
               makeVariable<T>(Dims{Dim::X}, Shape{5}, Values{1, 5, 4, 5, 1}));
    ds.setMask("masks_x",
               makeVariable<bool>(Dims{Dim::X}, Shape{5},
                                  Values{false, true, false, true, false}));
  }
  Dataset ds;
};

using DataTypes = ::testing::Types<double, float, int64_t, int32_t>;
TYPED_TEST_SUITE(DatasetShapeChangingOpTest, DataTypes);

TYPED_TEST(DatasetShapeChangingOpTest, sum_masked) {
  const auto result = sum(this->ds, Dim::X);

  ASSERT_EQ(result["data_x"].data(),
            makeVariable<TypeParam>(Values{TypeParam{6}}));
}

TYPED_TEST(DatasetShapeChangingOpTest, mean_masked) {
  const auto result = mean(this->ds, Dim::X);

  if constexpr (std::is_floating_point_v<TypeParam>)
    ASSERT_EQ(result["data_x"].data(),
              makeVariable<TypeParam>(Values{TypeParam{2}}));
  else // non floating point gets the result as a double
    ASSERT_EQ(result["data_x"].data(), makeVariable<double>(Values{double{2}}));
}

TYPED_TEST(DatasetShapeChangingOpTest, mean_fully_masked) {
  this->ds.setMask(
      "full_mask",
      makeVariable<bool>(Dimensions{Dim::X, 5}, Values(make_bools(5, true))));
  const Dataset result = mean(this->ds, Dim::X);

  if constexpr (std::is_floating_point_v<TypeParam>)
    EXPECT_TRUE(std::isnan(result["data_x"].values<TypeParam>()[0]));
  else
    EXPECT_TRUE(std::isnan(result["data_x"].values<double>()[0]));
}

TEST(DatasetOperationsTest, mean_two_dims) {
  Dataset ds;
  // the negative values should have been masked out
  ds.setData("data_xy", makeVariable<int64_t>(Dims{Dim::X, Dim::Y}, Shape{5, 2},
                                              Values{-999, -999, 3, -999, 5, 6,
                                                     -999, 10, 10, -999}));

  ds.setMask("mask_xy",
             makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{5, 2},
                                Values{true, true, false, true, false, false,
                                       true, false, false, true}));

  const Dataset result = mean(ds, Dim::X);

  ASSERT_EQ(result["data_xy"].data(),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{6, 8}));
}

TEST(DatasetOperationsTest, mean_three_dims) {
  Dataset ds;
  // the negative values should have been masked out
  ds.setData("data_xy",
             makeVariable<int64_t>(
                 Dims{Dim::Z, Dim::X, Dim::Y}, Shape{2, 5, 2},
                 Values{-999, -999, 3, -999, 5, 6, -999, 10, 10, -999,
                        -999, -999, 3, -999, 5, 6, -999, 10, 10, -999}));

  ds.setMask(
      "mask_xy",
      makeVariable<bool>(Dims{Dim::Z, Dim::X, Dim::Y}, Shape{2, 5, 2},
                         Values{true,  true,  false, true,  false, false, true,
                                false, false, true,  true,  true,  false, true,
                                false, false, true,  false, false, true}));

  const Dataset result = mean(ds, Dim::X);

  ASSERT_EQ(result["data_xy"].data(),
            makeVariable<double>(Dims{Dim::Z, Dim::Y}, Shape{2, 2},
                                 Values{6, 8, 6, 8}));
}
