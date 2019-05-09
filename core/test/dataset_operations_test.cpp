// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include "dataset.h"
#include "dimensions.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

class DataProxyBinaryOpEqualsTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<Dataset::value_type> {};

DatasetFactory3D datasetFactory;
auto d = datasetFactory.make();

INSTANTIATE_TEST_SUITE_P(AllItems, DataProxyBinaryOpEqualsTest,
                         ::testing::ValuesIn(d));

TEST_P(DataProxyBinaryOpEqualsTest, other_data_unchanged) {
  const auto &item = GetParam().second;
  auto dataset = datasetFactory.make();
  const auto original(dataset);
  const auto target = dataset["data_zyx"];

  ASSERT_NO_THROW(target += item);

  for (const auto & [ name, data ] : dataset) {
    if (name != "data_zyx") {
      EXPECT_EQ(data, original[name]);
    }
  }
}

TEST_P(DataProxyBinaryOpEqualsTest, plus_lhs_with_variance) {
  const auto &item = GetParam().second;
  auto dataset = datasetFactory.make();
  const auto target = dataset["data_zyx"];
  auto reference_values = target.values() + item.values();
  auto reference_variances = target.variances();
  if (item.hasVariances())
    reference_variances += item.variances();

  ASSERT_NO_THROW(target += item);

  EXPECT_EQ(target.values(), reference_values);
  EXPECT_EQ(target.variances(), reference_variances);
}

TEST_P(DataProxyBinaryOpEqualsTest, plus_lhs_without_variance) {
  const auto &item = GetParam().second;
  auto dataset = datasetFactory.make();
  const auto target = dataset["data_xyz"];
  auto reference_values = target.values() + item.values();
  if (item.hasVariances()) {
    ASSERT_ANY_THROW(target += item);
  } else {
    ASSERT_NO_THROW(target += item);
    EXPECT_EQ(target.values(), reference_values);
    EXPECT_FALSE(target.hasVariances());
  }
}

TEST_P(DataProxyBinaryOpEqualsTest, times_lhs_with_variance) {
  const auto &item = GetParam().second;
  auto dataset = datasetFactory.make();
  const auto target = dataset["data_zyx"];
  auto reference_values = target.values() * item.values();
  auto reference_variances =
      target.variances() * (item.values() * item.values());
  if (item.hasVariances())
    reference_variances +=
        (target.values() * target.values()) * item.variances();

  ASSERT_NO_THROW(target *= item);

  EXPECT_EQ(target.values(), reference_values);
  EXPECT_EQ(target.variances(), reference_variances);
}

TEST_P(DataProxyBinaryOpEqualsTest, times_lhs_without_variance) {
  const auto &item = GetParam().second;
  auto dataset = datasetFactory.make();
  const auto target = dataset["data_xyz"];
  auto reference_values = target.values() * item.values();
  if (item.hasVariances()) {
    ASSERT_ANY_THROW(target *= item);
  } else {
    ASSERT_NO_THROW(target *= item);
    EXPECT_EQ(target.values(), reference_values);
    EXPECT_FALSE(target.hasVariances());
  }
}

TEST_P(DataProxyBinaryOpEqualsTest, plus_slice_lhs_with_variance) {
  const auto &item = GetParam().second;
  auto dataset = datasetFactory.make();
  const auto target = dataset["data_zyx"];
  const auto &dims = item.dims();
  for (const Dim dim : dims.labels()) {
    auto reference_values = target.values() + item.values().slice({dim, 2});
    auto reference_variances = target.variances();
    if (item.hasVariances())
      reference_variances += item.variances().slice({dim, 2});

    // Fails if any *other* multi-dimensional coord/label also depends on the
    // slicing dimension, since it will have mismatching values. Note that this
    // behavior is intended and important. It is crucial for preventing
    // operations between misaligned data in case a coordinate is
    // multi-dimensional.
    const auto coords = item.coords();
    const auto labels = item.labels();
    if (std::all_of(coords.begin(), coords.end(),
                    [dim](const auto &coord) {
                      return coord.first == dim ||
                             !coord.second.dims().contains(dim);
                    }) &&
        std::all_of(labels.begin(), labels.end(), [dim](const auto &labels) {
          return labels.second.dims().inner() == dim ||
                 !labels.second.dims().contains(dim);
        })) {
      ASSERT_NO_THROW(target += item.slice({dim, 2}));

      EXPECT_EQ(target.values(), reference_values);
      EXPECT_EQ(target.variances(), reference_variances);
    } else {
      ASSERT_ANY_THROW(target += item.slice({dim, 2}));
    }
  }
}

// We need the decltype(auto) since we are using these operators for both
// proxies and non-proxies. The former return by reference, the latter by value.
struct plus_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a += b;
  }
};
struct times_equals {
  template <class A, class B>
  decltype(auto) operator()(A &&a, const B &b) const {
    return a *= b;
  }
};

template <class Op>
class DatasetBinaryOpTest : public ::testing::Test,
                            public ::testing::WithParamInterface<Op> {
protected:
  Op op;
};

using BinaryEquals = ::testing::Types<plus_equals, times_equals>;
TYPED_TEST_SUITE(DatasetBinaryOpTest, BinaryEquals);

// DataProxyBinaryOpEqualsTest ensures correctness of operations between
// DataProxy with itself, so we can rely on that for building the reference.
TYPED_TEST(DatasetBinaryOpTest, rhs_DataProxy_self_overlap) {
  auto dataset = datasetFactory.make();
  auto original(dataset);
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, dataset["data_scalar"]));
  for (const auto[name, item] : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[name], original["data_scalar"]));
  }
}

TYPED_TEST(DatasetBinaryOpTest, rhs_DataProxy_self_overlap_slice) {
  auto dataset = datasetFactory.make();
  auto original(dataset);
  auto reference(dataset);

  ASSERT_NO_THROW(
      TestFixture::op(dataset, dataset["values_x"].slice({Dim::X, 1})));
  for (const auto[name, item] : dataset) {
    EXPECT_EQ(item, TestFixture::op(reference[name],
                                    original["values_x"].slice({Dim::X, 1})));
  }
}

TYPED_TEST(DatasetBinaryOpTest, rhs_Dataset) {
  auto a = datasetFactory.make();
  auto b = datasetFactory.make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(a, b));
  for (const auto[name, item] : a) {
    EXPECT_EQ(item, TestFixture::op(reference[name], b[name]));
  }
}

TYPED_TEST(DatasetBinaryOpTest, rhs_Dataset_coord_mismatch) {
  auto a = datasetFactory.make();
  DatasetFactory3D otherCoordsFactory;
  auto b = otherCoordsFactory.make();

  ASSERT_THROW(TestFixture::op(a, b), except::CoordMismatchError);
}

TYPED_TEST(DatasetBinaryOpTest, rhs_Dataset_with_missing_items) {
  auto a = datasetFactory.make();
  a.setValues("extra", makeVariable<double>({}));
  auto b = datasetFactory.make();
  auto reference(a);

  ASSERT_NO_THROW(TestFixture::op(a, b));
  for (const auto[name, item] : a) {
    if (name == "extra") {
      EXPECT_EQ(item, reference[name]);
    } else {
      EXPECT_EQ(item, TestFixture::op(reference[name], b[name]));
    }
  }
}

TYPED_TEST(DatasetBinaryOpTest, rhs_Dataset_with_extra_items) {
  auto a = datasetFactory.make();
  auto b = datasetFactory.make();
  b.setValues("extra", makeVariable<double>({}));

  ASSERT_ANY_THROW(TestFixture::op(a, b));
}

TYPED_TEST(DatasetBinaryOpTest, rhs_DatasetProxy_self_overlap) {
  auto dataset = datasetFactory.make();
  const auto slice = dataset.slice({Dim::Z, 3});
  auto reference(dataset);

  ASSERT_NO_THROW(TestFixture::op(dataset, slice));
  for (const auto[name, item] : dataset) {
    // Items independent of Z are removed when creating `slice`.
    if (item.dims().contains(Dim::Z)) {
      EXPECT_EQ(item, TestFixture::op(reference[name],
                                      reference[name].slice({Dim::Z, 3})));
    } else {
      EXPECT_EQ(item, reference[name]);
    }
  }
}

TYPED_TEST(DatasetBinaryOpTest, rhs_DatasetProxy_coord_mismatch) {
  auto dataset = datasetFactory.make();

  // Non-range sliced throws for X and Y due to multi-dimensional coords.
  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::X, 3})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::Y, 3})),
               except::CoordMismatchError);

  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::X, 3, 4})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::Y, 3, 4})),
               except::CoordMismatchError);
  ASSERT_THROW(TestFixture::op(dataset, dataset.slice({Dim::Z, 3, 4})),
               except::CoordMismatchError);
}
