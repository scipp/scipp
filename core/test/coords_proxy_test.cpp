// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

template <typename T> class CoordsProxyTest : public ::testing::Test {
protected:
  template <class D>
  std::conditional_t<std::is_same_v<T, CoordsProxy>, Dataset, const Dataset> &
  access(D &dataset) {
    return dataset;
  }
};

using CoordsProxyTypes = ::testing::Types<CoordsProxy, CoordsConstProxy>;
TYPED_TEST_SUITE(CoordsProxyTest, CoordsProxyTypes);

TYPED_TEST(CoordsProxyTest, empty) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();
  ASSERT_TRUE(coords.empty());
  ASSERT_EQ(coords.size(), 0);
}

TYPED_TEST(CoordsProxyTest, bad_item_access) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();
  ASSERT_ANY_THROW(coords[Dim::X]);
}

TYPED_TEST(CoordsProxyTest, item_access) {
  Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {4, 5});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);

  const auto coords = TestFixture::access(d).coords();
  ASSERT_EQ(coords[Dim::X], x);
  ASSERT_EQ(coords[Dim::Y], y);
}

TYPED_TEST(CoordsProxyTest, sparse_coords_values_and_coords) {
  Dataset d;
  auto data = makeVariable<double>({Dim::X, Dimensions::Sparse});
  data.sparseValues<double>()[0] = {1, 2, 3};
  auto s_coords = makeVariable<double>({Dim::X, Dimensions::Sparse});
  s_coords.sparseValues<double>()[0] = {4, 5, 6};
  d.setData("test", data);
  d.setSparseCoord("test", s_coords);
  ASSERT_EQ(1, d["test"].coords().size());
  auto sparseX = d["test"].coords()[Dim::X].sparseValues<double>()[0];
  ASSERT_EQ(3, sparseX.size());
  ASSERT_EQ(scipp::core::sparse_container<double>({4, 5, 6}), sparseX);
}

TYPED_TEST(CoordsProxyTest, iterators_empty_coords) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();

  ASSERT_NO_THROW(coords.begin());
  ASSERT_NO_THROW(coords.end());
  EXPECT_EQ(coords.begin(), coords.end());
}

TYPED_TEST(CoordsProxyTest, iterators) {
  Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {4, 5});
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

TYPED_TEST(CoordsProxyTest, slice) {
  Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {1, 2});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  const auto coords = TestFixture::access(d).coords();

  const auto sliceX = coords.slice({Dim::X, 1});
  EXPECT_ANY_THROW(sliceX[Dim::X]);
  EXPECT_EQ(sliceX[Dim::Y], y);

  const auto sliceDX = coords.slice({Dim::X, 1, 2});
  EXPECT_EQ(sliceDX[Dim::X], x.slice({Dim::X, 1, 2}));
  EXPECT_EQ(sliceDX[Dim::Y], y);

  const auto sliceY = coords.slice({Dim::Y, 1});
  EXPECT_EQ(sliceY[Dim::X], x);
  EXPECT_ANY_THROW(sliceY[Dim::Y]);

  const auto sliceDY = coords.slice({Dim::Y, 1, 2});
  EXPECT_EQ(sliceDY[Dim::X], x);
  EXPECT_EQ(sliceDY[Dim::Y], y.slice({Dim::Y, 1, 2}));
}

TYPED_TEST(CoordsProxyTest, find_and_contains) {
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
  const auto x =
      makeVariable<double>({{Dim::X, 3}, {Dim::Y, 2}}, {1, 2, 3, 4, 5, 6});
  const auto y = makeVariable<double>({Dim::Y, 2}, {1, 2});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  return d;
}

TYPED_TEST(CoordsProxyTest, slice_2D_coord) {
  auto d = make_dataset_2d_coord_x_1d_coord_y();
  const auto coords = TestFixture::access(d).coords();

  const auto sliceX = coords.slice({Dim::X, 1});
  EXPECT_ANY_THROW(sliceX[Dim::X]);
  EXPECT_EQ(sliceX[Dim::Y], coords[Dim::Y]);

  const auto sliceDX = coords.slice({Dim::X, 1, 2});
  EXPECT_EQ(sliceDX[Dim::X], coords[Dim::X].slice({Dim::X, 1, 2}));
  EXPECT_EQ(sliceDX[Dim::Y], coords[Dim::Y]);

  const auto sliceY = coords.slice({Dim::Y, 1});
  EXPECT_EQ(sliceY[Dim::X], coords[Dim::X].slice({Dim::Y, 1}));
  EXPECT_ANY_THROW(sliceY[Dim::Y]);

  const auto sliceDY = coords.slice({Dim::Y, 1, 2});
  EXPECT_EQ(sliceDY[Dim::X], coords[Dim::X].slice({Dim::Y, 1, 2}));
  EXPECT_EQ(sliceDY[Dim::Y], coords[Dim::Y].slice({Dim::Y, 1, 2}));
}

auto check_slice_of_slice = [](const auto &dataset, const auto slice) {
  EXPECT_EQ(slice[Dim::X],
            dataset.coords()[Dim::X].slice({Dim::X, 1, 3}).slice({Dim::Y, 1}));
  EXPECT_ANY_THROW(slice[Dim::Y]);
};

TYPED_TEST(CoordsProxyTest, slice_of_slice) {
  auto d = make_dataset_2d_coord_x_1d_coord_y();
  const auto cs = TestFixture::access(d).coords();

  check_slice_of_slice(d, cs.slice({Dim::X, 1, 3}).slice({Dim::Y, 1}));
  check_slice_of_slice(d, cs.slice({Dim::Y, 1}).slice({Dim::X, 1, 3}));
  check_slice_of_slice(d, cs.slice({Dim::X, 1, 3}, {Dim::Y, 1}));
  check_slice_of_slice(d, cs.slice({Dim::Y, 1}, {Dim::X, 1, 3}));
}

auto check_slice_of_slice_range = [](const auto &dataset, const auto slice) {
  EXPECT_EQ(
      slice[Dim::X],
      dataset.coords()[Dim::X].slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 2}));
  EXPECT_EQ(slice[Dim::Y], dataset.coords()[Dim::Y].slice({Dim::Y, 1, 2}));
};

TYPED_TEST(CoordsProxyTest, slice_of_slice_range) {
  auto d = make_dataset_2d_coord_x_1d_coord_y();
  const auto cs = TestFixture::access(d).coords();

  check_slice_of_slice_range(d, cs.slice({Dim::X, 1, 3}).slice({Dim::Y, 1, 2}));
  check_slice_of_slice_range(d, cs.slice({Dim::Y, 1, 2}).slice({Dim::X, 1, 3}));
  check_slice_of_slice_range(d, cs.slice({Dim::X, 1, 3}, {Dim::Y, 1, 2}));
  check_slice_of_slice_range(d, cs.slice({Dim::Y, 1, 2}, {Dim::X, 1, 3}));
}

TEST(CoordsConstProxy, slice_return_type) {
  const Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(d.coords().slice({Dim::X, 0})),
                              CoordsConstProxy>));
}

TEST(CoordsProxy, slice_return_type) {
  Dataset d;
  ASSERT_TRUE(
      (std::is_same_v<decltype(d.coords().slice({Dim::X, 0})), CoordsProxy>));
}

TEST(MutableCoordsProxyTest, item_write) {
  Dataset d;
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 2}, {4, 5});
  const auto x_reference = makeVariable<double>({Dim::X, 3}, {1.5, 2.0, 3.0});
  const auto y_reference = makeVariable<double>({Dim::Y, 2}, {4.5, 5.0});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);

  const auto coords = d.coords();
  coords[Dim::X].values<double>()[0] += 0.5;
  coords[Dim::Y].values<double>()[0] += 0.5;
  ASSERT_EQ(coords[Dim::X], x_reference);
  ASSERT_EQ(coords[Dim::Y], y_reference);
}

TEST(CoordsProxy, modify_slice) {
  auto d = make_dataset_2d_coord_x_1d_coord_y();
  const auto coords = d.coords();

  const auto slice = coords.slice({Dim::X, 1, 2});
  for (auto &x : slice[Dim::X].values<double>())
    x = 0.0;

  const auto reference =
      makeVariable<double>({{Dim::X, 3}, {Dim::Y, 2}}, {1, 2, 0, 0, 5, 6});
  EXPECT_EQ(d.coords()[Dim::X], reference);
}

TEST(CoordsConstProxy, slice_bin_edges_with_2D_coord) {
  Dataset d;
  const auto x = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  const auto y_edges = makeVariable<double>({Dim::Y, 3}, {1, 2, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y_edges);
  const auto coords = d.coords();

  const auto sliceX = coords.slice({Dim::X, 1});
  EXPECT_ANY_THROW(sliceX[Dim::X]);
  EXPECT_EQ(sliceX[Dim::Y], coords[Dim::Y]);

  const auto sliceDX = coords.slice({Dim::X, 1, 2});
  EXPECT_EQ(sliceDX[Dim::X].dims(), Dimensions({{Dim::Y, 2}, {Dim::X, 1}}));
  EXPECT_EQ(sliceDX[Dim::Y], coords[Dim::Y]);

  const auto sliceY = coords.slice({Dim::Y, 1});
  // TODO Would it be more consistent to preserve X with 0 thickness?
  EXPECT_ANY_THROW(sliceY[Dim::X]);
  EXPECT_ANY_THROW(sliceY[Dim::Y]);

  const auto sliceY_edge = coords.slice({Dim::Y, 1, 2});
  EXPECT_EQ(sliceY_edge[Dim::X], coords[Dim::X].slice({Dim::Y, 1, 1}));
  EXPECT_EQ(sliceY_edge[Dim::Y], coords[Dim::Y].slice({Dim::Y, 1, 2}));

  const auto sliceY_bin = coords.slice({Dim::Y, 1, 3});
  EXPECT_EQ(sliceY_bin[Dim::X], coords[Dim::X].slice({Dim::Y, 1, 2}));
  EXPECT_EQ(sliceY_bin[Dim::Y], coords[Dim::Y].slice({Dim::Y, 1, 3}));
}
