// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "dataset_test_common.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/mean.h"
#include "scipp/dataset/nansum.h"
#include "scipp/dataset/sum.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(DatasetOperationsTest, sum_over_dim) {
  auto ds = make_1_values_and_variances<float>(
      "a", {Dim::X, 3}, sc_units::dimensionless, {1, 2, 3}, {12, 15, 18});
  EXPECT_EQ(sum(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{6}, Variances{45}));
  EXPECT_EQ(sum(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{3}, Variances{27}));
}

TEST(DatasetOperationsTest, sum_all_dims) {
  DataArray da{makeVariable<double>(Dims{Dim::X, Dim::Y}, Values{1, 1, 1, 1},
                                    Shape{2, 2})};
  EXPECT_EQ(sum(da).data(), makeVariable<double>(Values{4}));

  Dataset ds{{{"a", da}}};
  EXPECT_EQ(nansum(ds)["a"], nansum(da));
}

TEST(DatasetOperationsTest, sum_over_dim_empty_dataset) {
  auto ds = make_1_values_and_variances<float>(
      "a", {Dim::X, 3}, sc_units::dimensionless, {1, 2, 3}, {12, 15, 18});
  ds.erase("a");
  auto res = sum(ds, Dim::X);
  EXPECT_TRUE(res.is_valid());
  EXPECT_EQ(res, Dataset({}, {}));
}

TEST(DatasetOperationsTest, nansum_over_dim) {
  auto ds = make_1_values_and_variances<double>(
      "a", {Dim::X, 3}, sc_units::dimensionless, {1.0, 2.0, double(NAN)},
      {2.0, 5.0, 6.0});
  EXPECT_EQ(nansum(ds, Dim::X)["a"].data(),
            makeVariable<double>(Values{3}, Variances{7}));
}

TEST(DatasetOperationsTest, nansum_all_dims) {
  DataArray da{makeVariable<double>(
      Dims{Dim::X, Dim::Y}, Values{1.0, 1.0, double(NAN), 1.0}, Shape{2, 2})};
  EXPECT_EQ(nansum(da).data(), makeVariable<double>(Values{3}));

  Dataset ds{{{"a", da}}};
  EXPECT_EQ(nansum(ds)["a"], nansum(da));
}

TEST(DatasetOperationsTest, nansum_over_dim_empty_dataset) {
  auto ds = make_1_values_and_variances<float>(
      "a", {Dim::X, 3}, sc_units::dimensionless, {1, 2, 3}, {12, 15, 18});
  ds.erase("a");
  auto res = nansum(ds, Dim::X);
  EXPECT_TRUE(res.is_valid());
  EXPECT_EQ(res, Dataset({}, {}));
}

template <typename T>
class DatasetShapeChangingOpTest : public ::testing::Test {
public:
  void SetUp() {
    ds = Dataset({{"data_x", makeVariable<T>(Dims{Dim::X}, Shape{5},
                                             Values{1, 5, 4, 5, 1})}});
    ds["data_x"].masks().set(
        "masks_x", makeVariable<bool>(Dims{Dim::X}, Shape{5},
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
  this->ds["data_x"].masks().set(
      "full_mask",
      makeVariable<bool>(Dimensions{Dim::X, 5}, Values(make_bools(5, true))));
  const Dataset result = mean(this->ds, Dim::X);

  if constexpr (std::is_floating_point_v<TypeParam>)
    EXPECT_TRUE(std::isnan(result["data_x"].values<TypeParam>()[0]));
  else
    EXPECT_TRUE(std::isnan(result["data_x"].values<double>()[0]));
}

TEST(DatasetOperationsTest, mean_two_dims) {
  // the negative values should have been masked out
  Dataset ds(
      {{"data_xy", makeVariable<int64_t>(Dims{Dim::X, Dim::Y}, Shape{5, 2},
                                         Values{-999, -999, 3, -999, 5, 6, -999,
                                                10, 10, -999})}});

  ds["data_xy"].masks().set(
      "mask_xy", makeVariable<bool>(Dims{Dim::X, Dim::Y}, Shape{5, 2},
                                    Values{true, true, false, true, false,
                                           false, true, false, false, true}));

  const Dataset result = mean(ds, Dim::X);
  EXPECT_EQ(result["data_xy"].data(),
            makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{6, 8}));
}

TEST(DatasetOperationsTest, mean_three_dims) {
  // the negative values should have been masked out
  Dataset ds({{"data_xy",
               makeVariable<int64_t>(
                   Dims{Dim::Z, Dim::X, Dim::Y}, Shape{2, 5, 2},
                   Values{-999, -999, 3, -999, 5, 6, -999, 10, 10, -999,
                          -999, -999, 3, -999, 5, 6, -999, 10, 10, -999})}});

  ds["data_xy"].masks().set(
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
