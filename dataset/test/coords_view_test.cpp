// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"
#include "test_macros.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::dataset;

template <typename T> class CoordsViewTest : public ::testing::Test {
protected:
  template <class D>
  std::conditional_t<std::is_same_v<T, CoordsView>, Dataset, const Dataset> &
  access(D &dataset) {
    return dataset;
  }
};

using CoordsViewTypes = ::testing::Types<CoordsView, CoordsConstView>;
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

TYPED_TEST(CoordsViewTest, events_coords_values_and_coords) {
  Dataset d;
  auto data = makeVariable<event_list<double>>(Dims{}, Shape{});
  data.values<event_list<double>>()[0] = {1, 2, 3};
  auto s_coords = makeVariable<event_list<double>>(Dims{}, Shape{});
  s_coords.values<event_list<double>>()[0] = {4, 5, 6};
  d.setData("test", data);
  d.coords().set(Dim::X, s_coords);
  ASSERT_EQ(1, d["test"].coords().size());
  auto eventsX = d["test"].coords()[Dim::X].values<event_list<double>>()[0];
  ASSERT_EQ(3, eventsX.size());
  ASSERT_EQ(scipp::event_list<double>({4, 5, 6}), eventsX);
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
  EXPECT_EQ(it->first, Dim::X);
  EXPECT_EQ(it->second, x);

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, coords.end());
  EXPECT_EQ(it->first, Dim::Y);
  EXPECT_EQ(it->second, y);

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, coords.end());
}

TYPED_TEST(CoordsViewTest, find_and_contains) {
  DatasetFactory3D factory;
  auto dataset = factory.make();
  const auto coords = TestFixture::access(dataset).coords();

  EXPECT_EQ(coords.find(Dim::Q), coords.end());
  EXPECT_EQ(coords.find(Dim::Time)->first, Dim::Time);
  EXPECT_EQ(coords.find(Dim::Time)->second, coords[Dim::Time]);
  EXPECT_FALSE(coords.contains(Dim::Q));
  EXPECT_TRUE(coords.contains(Dim::Time));

  EXPECT_EQ(coords.find(Dim::X)->first, Dim::X);
  EXPECT_EQ(coords.find(Dim::X)->second, coords[Dim::X]);
}

auto make_dataset_2d_coord_x_1d_coord_y() {
  Dataset d;
  const auto x = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2},
                                      Values{1, 2, 3, 4, 5, 6});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{1, 2});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  return d;
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

  const auto coords = d.coords();
  coords[Dim::X].values<double>()[0] += 0.5;
  coords[Dim::Y].values<double>()[0] += 0.5;
  ASSERT_EQ(coords[Dim::X], x_reference);
  ASSERT_EQ(coords[Dim::Y], y_reference);
}
