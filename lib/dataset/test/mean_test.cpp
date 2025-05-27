// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/overloaded.h"
#include "scipp/dataset/mean.h"
#include "scipp/dataset/nanmean.h"
#include "scipp/dataset/sum.h"

#include "fix_typed_test_suite_warnings.h"
#include "test_nans.h"

using namespace scipp;

TEST(DatasetTest, sum_and_mean) {
  Dataset ds(
      {{"a", makeVariable<float>(Dimensions{Dim::X, 3}, sc_units::one,
                                 Values{1, 2, 3}, Variances{12, 15, 18})}});
  EXPECT_EQ(sum(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{6}, Variances{45}));
  EXPECT_EQ(sum(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{3}, Variances{27}));

  EXPECT_EQ(mean(ds, Dim::X)["a"].data(),
            makeVariable<float>(Values{2}, Variances{5.0}));
  EXPECT_EQ(mean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
            makeVariable<float>(Values{1.5}, Variances{6.75}));
}

namespace {

using MeanTestTypes = testing::Types<int32_t, int64_t, float, double>;
TYPED_TEST_SUITE(MeanTest, MeanTestTypes);

template <class T, class T2>
auto make_one_item_dataset(const std::string &name, const Dimensions &dims,
                           const sc_units::Unit unit,
                           const std::initializer_list<T2> &values,
                           const std::initializer_list<T2> &variances) {
  return Dataset(
      {{name, makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                              Values(values), Variances(variances))}});
}
template <class T, class T2>
auto make_one_item_dataset(const std::string &name, const Dimensions &dims,
                           const sc_units::Unit unit,
                           const std::initializer_list<T2> &values) {
  return Dataset({{name, makeVariable<T>(Dimensions(dims), sc_units::Unit(unit),
                                         Values(values))}});
}

template <typename Op> void test_masked_data_array_1_mask(Op op) {
  const auto var =
      makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
                           Values{1.0, 2.0, 3.0, 4.0});
  const auto mask =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("mask", mask);
  const auto meanX = makeVariable<double>(Dimensions{Dim::Y, 2}, sc_units::m,
                                          Values{1.0, 3.0});
  const auto meanY = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                          Values{2.0, 3.0});
  EXPECT_EQ(op(a, Dim::X).data(), meanX);
  EXPECT_EQ(op(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(op(a, Dim::X).masks().contains("mask"));
  EXPECT_TRUE(op(a, Dim::Y).masks().contains("mask"));
}

template <typename Op> void test_masked_data_array_2_masks(Op op) {
  const auto var =
      makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
                           Values{1.0, 2.0, 3.0, 4.0});
  const auto maskX =
      makeVariable<bool>(Dimensions{Dim::X, 2}, Values{false, true});
  const auto maskY =
      makeVariable<bool>(Dimensions{Dim::Y, 2}, Values{false, true});
  DataArray a(var);
  a.masks().set("x", maskX);
  a.masks().set("y", maskY);
  const auto meanX = makeVariable<double>(Dimensions{Dim::Y, 2}, sc_units::m,
                                          Values{1.0, 3.0});
  const auto meanY = makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                                          Values{1.0, 2.0});
  EXPECT_EQ(op(a, Dim::X).data(), meanX);
  EXPECT_EQ(op(a, Dim::Y).data(), meanY);
  EXPECT_FALSE(op(a, Dim::X).masks().contains("x"));
  EXPECT_TRUE(op(a, Dim::X).masks().contains("y"));
  EXPECT_TRUE(op(a, Dim::Y).masks().contains("x"));
  EXPECT_FALSE(op(a, Dim::Y).masks().contains("y"));
}

template <typename Op> void test_masked_data_array_nd_mask(Op op) {
  const auto var =
      makeVariable<double>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
                           Values{1.0, 2.0, 3.0, 4.0});
  // Just a single masked element
  const auto mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                       Values{false, true, false, false});
  DataArray a(var);
  a.masks().set("mask", mask);
  const auto meanX =
      makeVariable<double>(Dimensions{Dim::Y, 2}, sc_units::m,
                           Values{(1.0 + 0.0) / 1, (3.0 + 4.0) / 2});
  const auto meanY =
      makeVariable<double>(Dimensions{Dim::X, 2}, sc_units::m,
                           Values{(1.0 + 3.0) / 2, (0.0 + 4.0) / 1});
  const auto mean = makeVariable<double>(sc_units::m, Shape{1},
                                         Values{(1.0 + 0.0 + 3.0 + 4.0) / 3});
  EXPECT_EQ(op(a, Dim::X).data(), meanX);
  EXPECT_EQ(op(a, Dim::Y).data(), meanY);
  EXPECT_EQ(op(a).data(), mean);
  EXPECT_FALSE(op(a, Dim::X).masks().contains("mask"));
  EXPECT_FALSE(op(a, Dim::X).masks().contains("mask"));
  EXPECT_FALSE(op(a).masks().contains("mask"));
}

auto mean_func = overloaded{
    [](const auto &x, const auto &dim) { return mean(x, dim); },
    [](const auto &x, const auto &dim, auto &out) { return mean(x, dim, out); },
    [](const auto &x) { return mean(x); }};
auto nanmean_func =
    overloaded{[](const auto &x, const auto &dim) { return nanmean(x, dim); },
               [](const auto &x, const auto &dim, auto &out) {
                 return nanmean(x, dim, out);
               },
               [](const auto &x) { return nanmean(x); }};
} // namespace

TEST(MeanTest, masked_data_array) {
  test_masked_data_array_1_mask(mean_func);
  test_masked_data_array_1_mask(nanmean_func);
}

TEST(MeanTest, masked_data_array_two_masks) {
  test_masked_data_array_2_masks(mean_func);
  test_masked_data_array_2_masks(nanmean_func);
}

TEST(MeanTest, masked_data_array_md_masks) {
  test_masked_data_array_nd_mask(mean_func);
  test_masked_data_array_nd_mask(nanmean_func);
}

TYPED_TEST(MeanTest, nanmean_masked_data_with_nans) {
  if constexpr (!TestFixture::TestNans)
    GTEST_SKIP_(
        "Test skipped for non-FP types"); // If type does not support nans skip
  else {
    // Two Nans
    const auto var = makeVariable<TypeParam>(
        Dimensions{{Dim::Y, 2}, {Dim::X, 2}}, sc_units::m,
        Values{double(NAN), double(NAN), 3.0, 4.0});
    // Two masked element
    const auto mask = makeVariable<bool>(Dimensions{{Dim::Y, 2}, {Dim::X, 2}},
                                         Values{false, true, true, false});
    DataArray a(var);
    a.masks().set("mask", mask);
    // First element NaN, second NaN AND masked, third masked, forth non-masked
    // finite number
    const auto mean = makeVariable<typename TestFixture::ReturnType>(
        sc_units::m, Values{(0.0 + 0.0 + 0.0 + 4.0) / 1});
    EXPECT_EQ(nanmean(a).data(), mean);
  }
}

TYPED_TEST(MeanTest, mean_over_dim) {
  if constexpr (TestFixture::TestVariances) {
    // Test with variances
    auto ds = make_one_item_dataset<TypeParam>(
        "a", {Dim::X, 3}, sc_units::dimensionless, {1, 2, 3}, {12, 15, 18});
    EXPECT_EQ(mean(ds, Dim::X)["a"].data(),
              makeVariable<TypeParam>(Values{2}, Variances{5.0}));
    EXPECT_EQ(mean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
              makeVariable<TypeParam>(Values{1.5}, Variances{6.75}));
  } else {
    auto ds = make_one_item_dataset<TypeParam>(
        "a", {Dim::X, 3}, sc_units::dimensionless, {1, 2, 3});
    EXPECT_EQ(mean(ds, Dim::X)["a"].data(), makeVariable<double>(Values{2}));
    EXPECT_EQ(mean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
              makeVariable<double>(Values{1.5}));
  }
}

TYPED_TEST(MeanTest, mean_all_dims) {
  DataArray da{makeVariable<TypeParam>(Dims{Dim::X, Dim::Y}, Values{1, 2, 3, 4},
                                       Shape{2, 2})};

  // For all FP input dtypes, dtype is same on output. Integers are converted to
  // double FP precision.
  using RetType = typename TestFixture::ReturnType;
  EXPECT_EQ(mean(da).data(), makeVariable<RetType>(Values{2.5}));
  EXPECT_EQ(mean(da).data(), makeVariable<RetType>(Values{2.5}));

  Dataset ds{{{"a", da}}};
  EXPECT_EQ(mean(ds)["a"], mean(da));
}

TYPED_TEST(MeanTest, nanmean_over_dim) {
  if constexpr (TestFixture::TestVariances) {
    // Test variances and values
    auto ds = make_one_item_dataset<TypeParam>(
        "a", {Dim::X, 3}, sc_units::dimensionless, {1, 2, 3}, {12, 15, 18});
    EXPECT_EQ(nanmean(ds, Dim::X)["a"].data(),
              makeVariable<TypeParam>(Values{2}, Variances{5.0}));
    EXPECT_EQ(nanmean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
              makeVariable<TypeParam>(Values{1.5}, Variances{6.75}));
    // Set and test with NANS
    ds["a"].template values<TypeParam>()[2] = TypeParam(NAN);
    EXPECT_EQ(nanmean(ds, Dim::X)["a"].data(),
              makeVariable<TypeParam>(Values{1.5}, Variances{6.75}));
    EXPECT_EQ(nanmean(ds["a"], Dim::X).data(),
              makeVariable<TypeParam>(Values{1.5}, Variances{6.75}));
  } else {
    auto ds = make_one_item_dataset<TypeParam>(
        "a", {Dim::X, 3}, sc_units::dimensionless, {1, 2, 3});
    EXPECT_EQ(nanmean(ds, Dim::X)["a"].data(), makeVariable<double>(Values{2}));
    EXPECT_EQ(nanmean(ds.slice({Dim::X, 0, 2}), Dim::X)["a"].data(),
              makeVariable<double>(Values{1.5}));
  }
}

TYPED_TEST(MeanTest, nanmean_all_dims) {
  DataArray da{makeVariable<TypeParam>(
      Dims{Dim::X, Dim::Y}, Values{1.0, 2.0, 3.0, 4.0}, Shape{2, 2})};

  // For all FP input dtypes, dtype is same on output. Integers are converted to
  // double FP precision.
  EXPECT_EQ(nanmean(da).data(),
            makeVariable<typename TestFixture::ReturnType>(Values{2.5}));

  Dataset ds{{{"a", da}}};
  EXPECT_EQ(nanmean(ds)["a"], nanmean(da));

  if constexpr (TestFixture::TestNans) {
    da.values<TypeParam>()[3] = TypeParam(NAN);
    EXPECT_EQ(nanmean(da).data(), makeVariable<TypeParam>(Values{2.0}));
  }
}

TEST(NanMeanTest, masked) {
  DataArray da{makeVariable<double>(
      Dims{Dim::X}, Shape{5}, Values{double(NAN), 2.0, 3.0, double(NAN), 5.0})};
  da.masks().set("mask",
                 makeVariable<bool>(Dims{Dim::X}, Shape{5},
                                    Values{false, false, true, true, false}));
  EXPECT_EQ(nanmean(da, Dim::X), nanmean(da));
  EXPECT_EQ(nanmean(da, Dim::X).data(), makeVariable<double>(Values{3.5}));
}
