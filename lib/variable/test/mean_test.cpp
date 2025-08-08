// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/common/overloaded.h"
#include "scipp/core/eigen.h"
#include "scipp/core/except.h"
#include "scipp/variable/reduction.h"

#include "fix_typed_test_suite_warnings.h"
#include "test_macros.h"
#include "test_nans.h"

namespace {
using namespace scipp;

using MeanTestTypes = testing::Types<int32_t, int64_t, float, double>;
TYPED_TEST_SUITE(MeanTest, MeanTestTypes);

template <typename Op> void unknown_dim_fail(Op op) {
  const auto var =
      makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::m,
                           Values{1.0, 2.0, 3.0, 4.0});
  EXPECT_THROW(const auto view = op(var, Dim::Z), except::DimensionError);
}

template <typename TestFixture, typename Op> void basic(Op op) {
  const auto var = makeVariable<typename TestFixture::TestType>(
      Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::m,
      Values{1.0, 2.0, 3.0, 4.0});
  using RetType = typename TestFixture::ReturnType;
  const auto meanX = makeVariable<RetType>(Dims{Dim::Y}, Shape{2}, sc_units::m,
                                           Values{1.5, 3.5});
  const auto meanY = makeVariable<RetType>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                           Values{2.0, 3.0});
  EXPECT_EQ(op(var, Dim::X), meanX);
  EXPECT_EQ(op(var, Dim::Y), meanY);
}

template <typename TestFixture, typename Op> void basic_all_dims(Op op) {
  const auto var = makeVariable<typename TestFixture::TestType>(
      Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::m,
      Values{1.0, 2.0, 3.0, 4.0});
  const auto meanAll =
      makeVariable<typename TestFixture::ReturnType>(sc_units::m, Values{2.5});
  EXPECT_EQ(op(var), meanAll);
}

template <typename TestFixture, typename Op> void dtype_preservation(Op op) {
  const auto var = makeVariable<typename TestFixture::TestType>(
      Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::m,
      Values{1.0, 2.0, 3.0, 4.0});
  using RetType = typename TestFixture::ReturnType;
  const auto meanX = makeVariable<RetType>(Dims{Dim::Y}, Shape{2}, sc_units::m,
                                           Values{1.5, 3.5});
  const auto meanY = makeVariable<RetType>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                           Values{2.0, 3.0});
  EXPECT_EQ(op(var, Dim::X), meanX);
  EXPECT_EQ(op(var, Dim::Y), meanY);
}

template <typename TestFixture, typename Op>
void variances_as_standard_deviation_of_the_mean([[maybe_unused]] Op op) {
  if constexpr (TestFixture::TestVariances) {
    const auto var = makeVariable<typename TestFixture::TestType>(
        Dims{Dim::Y, Dim::X}, Shape{2, 2}, sc_units::m,
        Values{1.0, 2.0, 3.0, 4.0}, Variances{5.0, 6.0, 7.0, 8.0});

    using RetType = typename TestFixture::ReturnType;
    const auto meanX = makeVariable<RetType>(Dims{Dim::Y}, Shape{2},
                                             sc_units::m, Values{1.5, 3.5},
                                             Variances{0.5 * 5.5, 0.5 * 7.5});
    const auto meanY = makeVariable<RetType>(Dims{Dim::X}, Shape{2},
                                             sc_units::m, Values{2.0, 3.0},
                                             Variances{0.5 * 6.0, 0.5 * 7.0});
    EXPECT_EQ(op(var, Dim::X), meanX);
    EXPECT_EQ(op(var, Dim::Y), meanY);
  } else
    GTEST_SKIP_("Test type does not support variance testing");
}

auto mean_func =
    overloaded{[](const auto &var, const auto &dim) { return mean(var, dim); },
               [](const auto &var, const auto &dim, auto &out) -> Variable & {
                 return mean(var, dim, out);
               },
               [](const auto &var) { return mean(var); }};
auto nanmean_func = overloaded{
    [](const auto &var, const auto &dim) { return nanmean(var, dim); },
    [](const auto &var, const auto &dim, auto &out) -> Variable & {
      return nanmean(var, dim, out);
    },
    [](const auto &var) { return nanmean(var); }};
} // namespace

TEST(MeanTest, unknown_dim_fail) {
  unknown_dim_fail(mean_func);
  unknown_dim_fail(nanmean_func);
}

TYPED_TEST(MeanTest, basic) {
  basic<TestFixture>(mean_func);
  basic<TestFixture>(nanmean_func);
}

TYPED_TEST(MeanTest, basic_all_dims) {
  basic_all_dims<TestFixture>(mean_func);
  basic_all_dims<TestFixture>(nanmean_func);
}

TYPED_TEST(MeanTest, dtype_preservation) {
  dtype_preservation<TestFixture>(mean_func);
}

TYPED_TEST(MeanTest, variances_as_standard_deviation_of_the_mean) {
  variances_as_standard_deviation_of_the_mean<TestFixture>(mean_func);
  variances_as_standard_deviation_of_the_mean<TestFixture>(nanmean_func);
}

TYPED_TEST(MeanTest, nanmean_basic) {
  auto var = makeVariable<TypeParam>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                     sc_units::m, Values{1, 2, 3, 4});
  using RetType = typename TestFixture::ReturnType;
  const auto meanX = makeVariable<RetType>(Dims{Dim::Y}, Shape{2}, sc_units::m,
                                           Values{1.5, 3.5});
  const auto meanY = makeVariable<RetType>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                           Values{2.0, 3.0});
  EXPECT_EQ(nanmean(var, Dim::X), meanX);
  EXPECT_EQ(nanmean(var, Dim::Y), meanY);
  if constexpr (TestFixture::TestNans) {
    var.template values<TypeParam>().data()[3] = TypeParam{NAN};
    EXPECT_EQ(nanmean(var, Dim::X),
              makeVariable<RetType>(Dims{Dim::Y}, Shape{2}, sc_units::m,
                                    Values{1.5, 3.0}));
    EXPECT_EQ(nanmean(var, Dim::Y),
              makeVariable<RetType>(Dims{Dim::X}, Shape{2}, sc_units::m,
                                    Values{2.0, 2.0}));
    EXPECT_EQ(nanmean(var), makeVariable<RetType>(sc_units::m, Values{2.0}));
  }
}

TEST(MeanTest, vector) {
  const auto var = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X, Dim::Y}, Shape{2, 2},
      Values{Eigen::Vector3d{1, 1, 1}, Eigen::Vector3d{2, 2, 2},
             Eigen::Vector3d{3, 3, 3}, Eigen::Vector3d{4, 4, 4}});

  const auto meanXY =
      makeVariable<Eigen::Vector3d>(Values{Eigen::Vector3d{2.5, 2.5, 2.5}});
  EXPECT_EQ(mean(var), meanXY);
  EXPECT_EQ(nanmean(var), meanXY);

  const auto meanX = makeVariable<Eigen::Vector3d>(
      Dims{Dim::Y}, Shape{2},
      Values{Eigen::Vector3d{2, 2, 2}, Eigen::Vector3d{3, 3, 3}});
  EXPECT_EQ(mean(var, Dim::X), meanX);
  EXPECT_EQ(nanmean(var, Dim::X), meanX);

  const auto meanY = makeVariable<Eigen::Vector3d>(
      Dims{Dim::X}, Shape{2},
      Values{Eigen::Vector3d{1.5, 1.5, 1.5}, Eigen::Vector3d{3.5, 3.5, 3.5}});
  EXPECT_EQ(mean(var, Dim::Y), meanY);
  EXPECT_EQ(nanmean(var, Dim::Y), meanY);
}
