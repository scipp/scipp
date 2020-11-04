// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
  using dataset_type = std::conditional_t<std::is_same_v<T, DataArrayView>,
                                          Dataset, const Dataset>;
};

using DataArrayViewTypes = ::testing::Types<DataArrayView, DataArrayConstView>;
TYPED_TEST_SUITE(DataArrayViewTest, DataArrayViewTypes);

TYPED_TEST(DataArrayViewTest, name_ignored_in_comparison) {
  const auto var = makeVariable<double>(Values{1.0});
  Dataset d;
  d.setData("a", var);
  d.setData("b", var);
  typename TestFixture::dataset_type &d_ref(d);
  EXPECT_EQ(d_ref["a"], d_ref["b"]);
}

TYPED_TEST(DataArrayViewTest, dims) {
  Dataset d;
  const auto dense = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{1, 2});
  typename TestFixture::dataset_type &d_ref(d);

  d.setData("dense", dense);
  ASSERT_EQ(d_ref["dense"].dims(), dense.dims());
}

TYPED_TEST(DataArrayViewTest, dims_with_extra_coords) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{4, 5, 6});
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", var);

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
  EXPECT_EQ(d_ref["a"].unit(), units::kg);
  EXPECT_EQ(d_ref["b"].unit(), units::s);
}

TYPED_TEST(DataArrayViewTest, coords) {
  Dataset d;
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  d.coords().set(Dim::X, var);
  d.setData("a", var);

  typename TestFixture::dataset_type &d_ref(d);
  ASSERT_NO_THROW(d_ref["a"].coords());
  ASSERT_EQ(d_ref["a"].coords(), d.coords());
}

TYPED_TEST(DataArrayViewTest, coords_contains_only_relevant) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{4, 5, 6});
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", var);
  const auto coords = d_ref["a"].coords();

  ASSERT_NE(coords, d.coords());
  ASSERT_EQ(coords.size(), 1);
  ASSERT_NO_THROW(coords[Dim::X]);
  ASSERT_EQ(coords[Dim::X], x);
}

TYPED_TEST(DataArrayViewTest, coords_contains_only_relevant_2d_dropped) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", var);
  const auto coords = d_ref["a"].coords();

  ASSERT_NE(coords, d.coords());
  ASSERT_EQ(coords.size(), 1);
  ASSERT_NO_THROW(coords[Dim::X]);
  ASSERT_EQ(coords[Dim::X], x);
}

TYPED_TEST(DataArrayViewTest, coords_contains_only_relevant_2d) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{3});
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", var);
  const auto coords = d_ref["a"].coords();

  // This is a very special case which is probably unlikely to occur in
  // practice. If the coordinate depends on extra dimensions and the data is
  // not, it implies that the coordinate cannot be for this data item, so it
  // is dropped.
  ASSERT_NE(coords, d.coords());
  ASSERT_EQ(coords.size(), 0);
  ASSERT_FALSE(coords.contains(Dim::X));
}

TYPED_TEST(DataArrayViewTest, hasVariances) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  d.setData("a", makeVariable<double>(Values{double{}}));
  d.setData("b", makeVariable<double>(Values{1}, Variances{1}));
  ASSERT_FALSE(d_ref["a"].hasVariances());
  ASSERT_TRUE(d_ref["b"].hasVariances());
}

TYPED_TEST(DataArrayViewTest, values_variances) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2},
                                        Variances{3, 4});
  d.setData("a", var);

  ASSERT_EQ(d_ref["a"].data(), var);
  ASSERT_TRUE(equals(d_ref["a"].template values<double>(), {1, 2}));
  ASSERT_TRUE(equals(d_ref["a"].template variances<double>(), {3, 4}));
  ASSERT_ANY_THROW(d_ref["a"].template values<float>());
  ASSERT_ANY_THROW(d_ref["a"].template variances<float>());
}
