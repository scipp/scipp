// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"

#include "dataset_test_common.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::dataset;

// Using typed tests for common functionality of DataArrayView and
// DataArrayConstView.
template <typename T> class DataArrayViewTest : public ::testing::Test {
protected:
  using dataset_type =
      std::conditional_t<std::is_same_v<T, DataArray>, Dataset, const Dataset>;
};

using DataArrayViewTypes = ::testing::Types<DataArray, const DataArray>;
TYPED_TEST_SUITE(DataArrayViewTest, DataArrayViewTypes);

TYPED_TEST(DataArrayViewTest, name_ignored_in_comparison) {
  const auto var = makeVariable<double>(Values{1.0});
  Dataset d({{"a", var}, {"b", var}});
  typename TestFixture::dataset_type &d_ref(d);
  EXPECT_EQ(d_ref["a"], d_ref["b"]);
}

TYPED_TEST(DataArrayViewTest, dims) {
  const auto dense = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{1, 2});
  Dataset d({{"dense", dense}});
  typename TestFixture::dataset_type &d_ref(d);
  ASSERT_EQ(d_ref["dense"].dims(), dense.dims());
}

TYPED_TEST(DataArrayViewTest, dims_with_extra_coords) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{4, 5, 6});
  const auto var = x * y;
  Dataset d({{"a", var}}, {{Dim::X, x}, {Dim::Y, y}});
  typename TestFixture::dataset_type &d_ref(d);
  ASSERT_EQ(d_ref["a"].dims(), var.dims());
}

TYPED_TEST(DataArrayViewTest, dtype) {
  Dataset d = testdata::make_dataset_x();
  typename TestFixture::dataset_type &d_ref(d);
  EXPECT_EQ(d_ref["a"].dtype(), dtype<double>);
  EXPECT_EQ(d_ref["b"].dtype(), dtype<int32_t>);
}

TYPED_TEST(DataArrayViewTest, unit) {
  Dataset d = testdata::make_dataset_x();
  typename TestFixture::dataset_type &d_ref(d);
  EXPECT_EQ(d_ref["a"].unit(), sc_units::kg);
  EXPECT_EQ(d_ref["b"].unit(), sc_units::s);
}

TYPED_TEST(DataArrayViewTest, coords) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  Dataset d({{"a", var}}, {{Dim::X, var}});

  typename TestFixture::dataset_type &d_ref(d);
  ASSERT_NO_THROW(d_ref["a"].coords());
  ASSERT_EQ(d_ref["a"].coords(), d.coords());
}

TYPED_TEST(DataArrayViewTest, has_variances) {
  Dataset d({{"a", makeVariable<double>(Values{double{}})},
             {"b", makeVariable<double>(Values{1}, Variances{1})}});
  typename TestFixture::dataset_type &d_ref(d);
  ASSERT_FALSE(d_ref["a"].has_variances());
  ASSERT_TRUE(d_ref["b"].has_variances());
}

TYPED_TEST(DataArrayViewTest, values_variances) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2},
                                        Variances{3, 4});
  Dataset d({{"a", var}});
  typename TestFixture::dataset_type &d_ref(d);

  ASSERT_EQ(d_ref["a"].data(), var);
  ASSERT_TRUE(equals(d_ref["a"].template values<double>(), {1, 2}));
  ASSERT_TRUE(equals(d_ref["a"].template variances<double>(), {3, 4}));
  ASSERT_ANY_THROW(d_ref["a"].template values<float>());
  ASSERT_ANY_THROW(d_ref["a"].template variances<float>());
}
