// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/isnan.h"
#include "scipp/variable/bins.h"

using namespace scipp;
using namespace scipp::dataset;

class DataArrayBinsReductionTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 4};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 2}, std::pair{2, 3},
                   std::pair{4, 6}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable binned_var = make_bins(indices, Dim::X, copy(buffer));
  DataArray binned_da{binned_var};
};

TEST_F(DataArrayBinsReductionTest, sum) {
  EXPECT_EQ(bins_sum(binned_da), DataArray(makeVariable<double>(
                                     indices.dims(), Values{3, 0, 3, 11})));
}

TEST_F(DataArrayBinsReductionTest, max) {
  EXPECT_EQ(bins_max(binned_da),
            DataArray(makeVariable<double>(
                indices.dims(),
                Values{2.0, std::numeric_limits<double>::lowest(), 3.0, 6.0})));
}

TEST_F(DataArrayBinsReductionTest, min) {
  EXPECT_EQ(bins_min(binned_da),
            DataArray(makeVariable<double>(
                indices.dims(),
                Values{1.0, std::numeric_limits<double>::max(), 3.0, 5.0})));
}

TEST_F(DataArrayBinsReductionTest, mean) {
  const auto res = bins_mean(binned_da);
  EXPECT_EQ(res.slice({Dim::Y, 0}),
            DataArray(makeVariable<double>(Dims{}, Values{1.5})));
  EXPECT_TRUE(isnan(res.slice({Dim::Y, 1})).data().value<bool>());
  EXPECT_EQ(res.slice({Dim::Y, 2}),
            DataArray(makeVariable<double>(Dims{}, Values{3.0})));
  EXPECT_EQ(res.slice({Dim::Y, 3}),
            DataArray(makeVariable<double>(Dims{}, Values{5.5})));
}

class DataArrayBinsNaNReductionTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 4};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 2}, std::pair{2, 3},
                   std::pair{3, 5}});
  Variable data = makeVariable<double>(
      Dims{Dim::X}, Shape{5},
      Values{1.0, std::numeric_limits<double>::quiet_NaN(),
             std::numeric_limits<double>::quiet_NaN(), 4.0, 5.0});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}});
  Variable binned_var = make_bins(indices, Dim::X, copy(buffer));
  DataArray binned_da{binned_var};
};

TEST_F(DataArrayBinsNaNReductionTest, nansum) {
  EXPECT_EQ(bins_nansum(binned_da), DataArray(makeVariable<double>(
                                        indices.dims(), Values{1, 0, 0, 9})));
}

TEST_F(DataArrayBinsNaNReductionTest, nanmax) {
  EXPECT_EQ(
      bins_nanmax(binned_da),
      DataArray(makeVariable<double>(
          indices.dims(), Values{1.0, std::numeric_limits<double>::lowest(),
                                 std::numeric_limits<double>::lowest(), 5.0})));
}

TEST_F(DataArrayBinsNaNReductionTest, nanmin) {
  EXPECT_EQ(
      bins_nanmin(binned_da),
      DataArray(makeVariable<double>(
          indices.dims(), Values{1.0, std::numeric_limits<double>::max(),
                                 std::numeric_limits<double>::max(), 4.0})));
}

TEST_F(DataArrayBinsNaNReductionTest, nanmean) {
  const auto res = bins_nanmean(binned_da);
  EXPECT_EQ(res.slice({Dim::Y, 0}),
            DataArray(makeVariable<double>(Dims{}, Values{1.0})));
  EXPECT_TRUE(isnan(res.slice({Dim::Y, 1})).data().value<bool>());
  EXPECT_TRUE(isnan(res.slice({Dim::Y, 2})).data().value<bool>());
  EXPECT_EQ(res.slice({Dim::Y, 3}),
            DataArray(makeVariable<double>(Dims{}, Values{4.5})));
}

class DataArrayBoolBinsReductionTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 4};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 2}, std::pair{2, 3},
                   std::pair{4, 6}});
  Variable data = makeVariable<bool>(
      Dims{Dim::X}, Shape{6}, Values{true, false, false, true, true, true});
  DataArray buffer = DataArray(data);
  Variable binned_var = make_bins(indices, Dim::X, copy(buffer));
  DataArray binned_da{binned_var};
};

TEST_F(DataArrayBoolBinsReductionTest, sum) {
  EXPECT_EQ(bins_sum(binned_da),
            DataArray(makeVariable<int64_t>(indices.dims(), Values{1, 0, 0, 2},
                                            sc_units::none)));
}

TEST_F(DataArrayBoolBinsReductionTest, max) {
  EXPECT_EQ(bins_max(binned_da),
            DataArray(makeVariable<bool>(indices.dims(),
                                         Values{true, false, false, true})));
}

TEST_F(DataArrayBoolBinsReductionTest, min) {
  EXPECT_EQ(bins_min(binned_da),
            DataArray(makeVariable<bool>(indices.dims(),
                                         Values{false, true, false, true})));
}

TEST_F(DataArrayBoolBinsReductionTest, all) {
  EXPECT_EQ(bins_all(binned_da),
            DataArray(makeVariable<bool>(indices.dims(),
                                         Values{false, true, false, true})));
}

TEST_F(DataArrayBoolBinsReductionTest, any) {
  EXPECT_EQ(bins_any(binned_da),
            DataArray(makeVariable<bool>(indices.dims(),
                                         Values{true, false, false, true})));
}

class DataArrayBinsMaskedReductionTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 3};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 3}, std::pair{4, 6}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{6}, Values{1, 2, 3, 4, 5, 6});
  Variable mask = makeVariable<bool>(
      Dims{Dim::X}, Shape{6}, Values{true, false, true, false, true, true});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}}, {{"m", mask}});
  Variable binned_var = make_bins(indices, Dim::X, copy(buffer));
  DataArray binned_da{binned_var};
};

TEST_F(DataArrayBinsMaskedReductionTest, sum) {
  EXPECT_EQ(bins_sum(binned_da),
            DataArray(makeVariable<double>(indices.dims(), Values{2, 0, 0})));
}

TEST_F(DataArrayBinsMaskedReductionTest, sum_with_variances) {
  buffer.data().setVariances(buffer.data());
  auto da = DataArray(make_bins(indices, Dim::X, copy(buffer)));
  ASSERT_TRUE(buffer.has_variances());
  EXPECT_EQ(bins_sum(da),
            DataArray(makeVariable<double>(indices.dims(), Values{2, 0, 0},
                                           Variances{2, 0, 0})));
}

TEST_F(DataArrayBinsMaskedReductionTest, max) {
  EXPECT_EQ(
      bins_max(binned_da),
      DataArray(makeVariable<double>(
          indices.dims(), Values{2.0, std::numeric_limits<double>::lowest(),
                                 std::numeric_limits<double>::lowest()})));
}

TEST_F(DataArrayBinsMaskedReductionTest, min) {
  EXPECT_EQ(bins_min(binned_da),
            DataArray(makeVariable<double>(
                indices.dims(), Values{2.0, std::numeric_limits<double>::max(),
                                       std::numeric_limits<double>::max()})));
}

TEST_F(DataArrayBinsMaskedReductionTest, mean) {
  const auto res = bins_mean(binned_da);
  EXPECT_EQ(res.slice({Dim::Y, 0}),
            DataArray(makeVariable<double>(Dims{}, Values{2})));
  EXPECT_TRUE(isnan(res.slice({Dim::Y, 1})).data().value<bool>());
  EXPECT_TRUE(isnan(res.slice({Dim::Y, 2})).data().value<bool>());
}

class DataArrayBinsMaskedNaNReductionTest : public ::testing::Test {
protected:
  Dimensions dims{Dim::Y, 3};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 3}, std::pair{3, 7}});
  Variable data = makeVariable<double>(
      Dims{Dim::X}, Shape{7},
      Values{std::numeric_limits<double>::quiet_NaN(), 2.0,
             std::numeric_limits<double>::quiet_NaN(), 4.0, 5.0,
             std::numeric_limits<double>::quiet_NaN(), 7.0});
  Variable mask =
      makeVariable<bool>(Dims{Dim::X}, Shape{7},
                         Values{true, false, false, true, false, false, false});
  DataArray buffer = DataArray(data, {{Dim::X, data + data}}, {{"m", mask}});
  Variable binned_var = make_bins(indices, Dim::X, copy(buffer));
  DataArray binned_da{binned_var};
};

TEST_F(DataArrayBinsMaskedNaNReductionTest, nansum) {
  EXPECT_EQ(bins_nansum(binned_da),
            DataArray(makeVariable<double>(indices.dims(), Values{2, 0, 12})));
}

TEST_F(DataArrayBinsMaskedNaNReductionTest, nanmax) {
  EXPECT_EQ(bins_nanmax(binned_da),
            DataArray(makeVariable<double>(
                indices.dims(),
                Values{2.0, std::numeric_limits<double>::lowest(), 7.0})));
}

TEST_F(DataArrayBinsMaskedNaNReductionTest, nanmin) {
  EXPECT_EQ(bins_nanmin(binned_da),
            DataArray(makeVariable<double>(
                indices.dims(),
                Values{2.0, std::numeric_limits<double>::max(), 5.0})));
}

TEST_F(DataArrayBinsMaskedNaNReductionTest, nanmean) {
  const auto res = bins_nanmean(binned_da);
  EXPECT_EQ(res.slice({Dim::Y, 0}),
            DataArray(makeVariable<double>(Dims{}, Values{2})));
  EXPECT_TRUE(isnan(res.slice({Dim::Y, 1})).data().value<bool>());
  EXPECT_EQ(res.slice({Dim::Y, 2}),
            DataArray(makeVariable<double>(Dims{}, Values{6})));
}
