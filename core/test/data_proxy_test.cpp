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

// Using typed tests for common functionality of DataProxy and DataConstProxy.
template <typename T> class DataProxyTest : public ::testing::Test {
protected:
  using dataset_type =
      std::conditional_t<std::is_same_v<T, DataProxy>, Dataset, const Dataset>;
};

using DataProxyTypes = ::testing::Types<DataProxy, DataConstProxy>;
TYPED_TEST_SUITE(DataProxyTest, DataProxyTypes);

TYPED_TEST(DataProxyTest, name_ignored_in_comparison) {
  const auto var = makeVariable<double>(1.0);
  Dataset d;
  d.setData("a", var);
  d.setData("b", var);
  typename TestFixture::dataset_type &d_ref(d);
  EXPECT_EQ(d_ref["a"], d_ref["b"]);
}

TYPED_TEST(DataProxyTest, sparse_sparseDim) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);

  d.setData("dense", makeVariable<double>({}));
  ASSERT_FALSE(d_ref["dense"].dims().sparse());
  ASSERT_EQ(d_ref["dense"].dims().sparseDim(), Dim::Invalid);

  d.setData("sparse_data",
            makeVariable<double>({Dim::X}, {Dimensions::Sparse}));
  ASSERT_TRUE(d_ref["sparse_data"].dims().sparse());
  ASSERT_EQ(d_ref["sparse_data"].dims().sparseDim(), Dim::X);

  d.setSparseCoord("sparse_coord",
                   makeVariable<double>({Dim::X}, {Dimensions::Sparse}));
  ASSERT_TRUE(d_ref["sparse_coord"].dims().sparse());
  ASSERT_EQ(d_ref["sparse_coord"].dims().sparseDim(), Dim::X);
}

TYPED_TEST(DataProxyTest, dims) {
  Dataset d;
  const auto dense = makeVariable<double>({{Dim::X, 1}, {Dim::Y, 2}});
  const auto sparse = makeVariable<double>({Dim::X, Dim::Y, Dim::Z},
                                           {1, 2, Dimensions::Sparse});
  typename TestFixture::dataset_type &d_ref(d);

  d.setData("dense", dense);
  ASSERT_EQ(d_ref["dense"].dims(), dense.dims());

  d.setData("sparse_data", sparse);
  ASSERT_EQ(d_ref["sparse_data"].dims(), sparse.dims());

  d.setSparseCoord("sparse_coord", sparse);
  ASSERT_EQ(d_ref["sparse_coord"].dims(), sparse.dims());
}

TYPED_TEST(DataProxyTest, dims_with_extra_coords) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", var);

  ASSERT_EQ(d_ref["a"].dims(), var.dims());
}

TYPED_TEST(DataProxyTest, unit) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);

  d.setData("dense", makeVariable<double>({}));
  EXPECT_EQ(d_ref["dense"].unit(), units::dimensionless);
}

TYPED_TEST(DataProxyTest, unit_access_fails_without_values) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  d.setSparseCoord("sparse",
                   makeVariable<double>({Dim::X}, {Dimensions::Sparse}));
  EXPECT_THROW(d_ref["sparse"].unit(), except::SparseDataError);
}

TYPED_TEST(DataProxyTest, coords) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, var);
  d.setData("a", var);

  ASSERT_NO_THROW(d_ref["a"].coords());
  ASSERT_EQ(d_ref["a"].coords(), d.coords());
}

TYPED_TEST(DataProxyTest, coords_sparse) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto var =
      makeVariable<double>({Dim::X, Dim::Y}, {3, Dimensions::Sparse});
  d.setSparseCoord("a", var);

  ASSERT_NO_THROW(d_ref["a"].coords());
  ASSERT_NE(d_ref["a"].coords(), d.coords());
  ASSERT_EQ(d_ref["a"].coords().size(), 1);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::Y]);
  ASSERT_EQ(d_ref["a"].coords()[Dim::Y], var);
}

TYPED_TEST(DataProxyTest, coords_sparse_shadow) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto sparse =
      makeVariable<double>({Dim::X, Dim::Y}, {3, Dimensions::Sparse});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setSparseCoord("a", sparse);

  ASSERT_NO_THROW(d_ref["a"].coords());
  ASSERT_NE(d_ref["a"].coords(), d.coords());
  ASSERT_EQ(d_ref["a"].coords().size(), 2);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::X]);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::Y]);
  ASSERT_EQ(d_ref["a"].coords()[Dim::X], x);
  ASSERT_NE(d_ref["a"].coords()[Dim::Y], y);
  ASSERT_EQ(d_ref["a"].coords()[Dim::Y], sparse);
}

TYPED_TEST(DataProxyTest, coords_sparse_shadow_even_if_no_coord) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto sparse =
      makeVariable<double>({Dim::X, Dim::Y}, {3, Dimensions::Sparse});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", sparse);

  ASSERT_NO_THROW(d_ref["a"].coords());
  // Dim::Y is sparse, so the global (non-sparse) Y coordinate does not make
  // sense and is thus hidden.
  ASSERT_NE(d_ref["a"].coords(), d.coords());
  ASSERT_EQ(d_ref["a"].coords().size(), 1);
  ASSERT_NO_THROW(d_ref["a"].coords()[Dim::X]);
  ASSERT_ANY_THROW(d_ref["a"].coords()[Dim::Y]);
  ASSERT_EQ(d_ref["a"].coords()[Dim::X], x);
}

TYPED_TEST(DataProxyTest, coords_contains_only_relevant) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({Dim::Y, 3}, {4, 5, 6});
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", var);
  const auto coords = d_ref["a"].coords();

  ASSERT_NE(coords, d.coords());
  ASSERT_EQ(coords.size(), 1);
  ASSERT_NO_THROW(coords[Dim::X]);
  ASSERT_EQ(coords[Dim::X], x);
}

TYPED_TEST(DataProxyTest, coords_contains_only_relevant_2d_dropped) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  const auto y = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", var);
  const auto coords = d_ref["a"].coords();

  ASSERT_NE(coords, d.coords());
  ASSERT_EQ(coords.size(), 1);
  ASSERT_NO_THROW(coords[Dim::X]);
  ASSERT_EQ(coords[Dim::X], x);
}

TYPED_TEST(DataProxyTest,
           coords_contains_only_relevant_2d_not_dropped_inconsistency) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto x = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
  const auto y = makeVariable<double>({Dim::Y, 3});
  const auto var = makeVariable<double>({Dim::X, 3});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Y, y);
  d.setData("a", var);
  const auto coords = d_ref["a"].coords();

  // This is a very special case which is probably unlikely to occur in
  // practice. If the coordinate depends on extra dimensions and the data is
  // not, it implies that the coordinate cannot be for this data item, so it
  // should be dropped... HOWEVER, the current implementation DOES NOT DROP IT.
  // Should that be changed?
  ASSERT_NE(coords, d.coords());
  ASSERT_EQ(coords.size(), 1);
  ASSERT_NO_THROW(coords[Dim::X]);
  ASSERT_EQ(coords[Dim::X], x);
}

TYPED_TEST(DataProxyTest, hasData_hasVariances) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);

  d.setData("a", makeVariable<double>({}));
  d.setData("b", makeVariable<double>(1, 1));

  ASSERT_TRUE(d_ref["a"].hasData());
  ASSERT_FALSE(d_ref["a"].hasVariances());

  ASSERT_TRUE(d_ref["b"].hasData());
  ASSERT_TRUE(d_ref["b"].hasVariances());
}

TYPED_TEST(DataProxyTest, isHistogram) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);

  d.setCoord(Dim::X, makeVariable<int>({Dim::X, 5}, {1, 2, 3, 4, 5}));

  d.setData("histogram", makeVariable<int>({Dim::X, 4}, {1, 2, 3, 4}));
  d.setData("point", makeVariable<int>({Dim::X, 5}, {1, 2, 3, 4, 5}));

  ASSERT_TRUE(d_ref["histogram"].isHistogram(Dim::X));
  ASSERT_FALSE(d_ref["point"].isHistogram(Dim::X));
}

TYPED_TEST(DataProxyTest, values_variances) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  const auto var = makeVariable<double>({Dim::X, 2}, {1, 2}, {3, 4});
  d.setData("a", var);

  ASSERT_EQ(d_ref["a"].data(), var);
  ASSERT_TRUE(equals(d_ref["a"].template values<double>(), {1, 2}));
  ASSERT_TRUE(equals(d_ref["a"].template variances<double>(), {3, 4}));
  ASSERT_ANY_THROW(d_ref["a"].template values<float>());
  ASSERT_ANY_THROW(d_ref["a"].template variances<float>());
}

TYPED_TEST(DataProxyTest, sparse_with_no_data) {
  Dataset d;
  typename TestFixture::dataset_type &d_ref(d);
  d.setSparseCoord("a", makeVariable<double>({Dim::X}, {Dimensions::Sparse}));

  EXPECT_ANY_THROW(d_ref["a"].data());
  ASSERT_FALSE(d_ref["a"].hasData());
  ASSERT_FALSE(d_ref["a"].hasVariances());
}
