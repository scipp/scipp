// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"
#include "scipp/variable/shape.h"
#include "test_macros.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::dataset;

template <typename T> class CoordsViewTest : public ::testing::Test {
protected:
  template <class D>
  std::conditional_t<std::is_same_v<T, Coords>, Dataset, const Dataset> &
  access(D &dataset) {
    return dataset;
  }
};

using CoordsViewTypes = ::testing::Types<Coords, const Coords>;
TYPED_TEST_SUITE(CoordsViewTest, CoordsViewTypes);

TYPED_TEST(CoordsViewTest, empty) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();
  ASSERT_TRUE(coords.empty());
  ASSERT_EQ(coords.size(), 0);
}

TYPED_TEST(CoordsViewTest, bad_item_access) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();
  ASSERT_ANY_THROW(coords[Dim::X]);
}

TYPED_TEST(CoordsViewTest, item_access) {
  Dataset d;
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{4, 5});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);

  const auto coords = TestFixture::access(d).coords();
  ASSERT_EQ(coords[Dim::X], x);
  ASSERT_EQ(coords[Dim::Y], y);
}

TYPED_TEST(CoordsViewTest, iterators_empty_coords) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();

  ASSERT_NO_THROW(coords.begin());
  ASSERT_NO_THROW(coords.end());
  EXPECT_EQ(coords.begin(), coords.end());
}

TYPED_TEST(CoordsViewTest, iterators) {
  Dataset d;
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{4, 5});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  const auto coords = TestFixture::access(d).coords();

  ASSERT_NO_THROW(coords.begin());
  ASSERT_NO_THROW(coords.end());

  auto it = coords.begin();
  ASSERT_NE(it, coords.end());
  if (it->first == Dim::X)
    EXPECT_EQ(it->second, x);
  else
    EXPECT_EQ(it->second, y);

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, coords.end());
  if (it->first == Dim::Y)
    EXPECT_EQ(it->second, y);
  else
    EXPECT_EQ(it->second, x);

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, coords.end());
}

TYPED_TEST(CoordsViewTest, find_and_contains) {
  DatasetFactory3D factory;
  auto dataset = factory.make();
  const auto coords = TestFixture::access(dataset).coords();

  EXPECT_EQ(coords.find(Dim::Row), coords.end());
  EXPECT_EQ(coords.find(Dim::Time)->first, Dim::Time);
  EXPECT_EQ(coords.find(Dim::Time)->second, coords[Dim::Time]);
  EXPECT_FALSE(coords.contains(Dim::Row));
  EXPECT_TRUE(coords.contains(Dim::Time));

  EXPECT_EQ(coords.find(Dim::X)->first, Dim::X);
  EXPECT_EQ(coords.find(Dim::X)->second, coords[Dim::X]);
}

TEST(MutableCoordsViewTest, item_write) {
  Dataset d;
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{4, 5});
  const auto x_reference =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.5, 2.0, 3.0});
  const auto y_reference =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{4.5, 5.0});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);

  auto &coords = d.coords();
  coords[Dim::X].values<double>()[0] += 0.5;
  coords[Dim::Y].values<double>()[0] += 0.5;
  ASSERT_EQ(coords[Dim::X], x_reference);
  ASSERT_EQ(coords[Dim::Y], y_reference);
}

TEST(DictTest, set_bin_edges) {
  const auto x2y3 = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3});
  const auto x2y4 = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 4});
  const auto x3y3 = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 3});
  const auto x3y4 = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 4});
  // Single bin in extra dim, not dim of data
  const auto x2_extra = makeVariable<double>(Dims{Dim::X, Dim::Z}, Shape{2, 2});
  const auto x3_extra = makeVariable<double>(Dims{Dim::X, Dim::Z}, Shape{3, 2});
  DataArray da(x2y3);
  ASSERT_NO_THROW(da.coords().set(Dim::X, x2y4));
  ASSERT_NO_THROW(da.coords().set(Dim::X, transpose(x2y4)));
  ASSERT_NO_THROW(da.coords().set(Dim::X, x3y3));
  ASSERT_NO_THROW(da.coords().set(Dim::X, transpose(x3y3)));
  ASSERT_NO_THROW(da.coords().set(Dim::X, x2_extra));
  ASSERT_NO_THROW(da.coords().set(Dim::X, transpose(x2_extra)));
  ASSERT_THROW(da.coords().set(Dim::X, x3y4), except::DimensionError);
  ASSERT_THROW(da.coords().set(Dim::X, transpose(x3y4)),
               except::DimensionError);
  ASSERT_THROW(da.coords().set(Dim::X, x3_extra), except::DimensionError);
  ASSERT_THROW(da.coords().set(Dim::X, transpose(x3_extra)),
               except::DimensionError);
}
