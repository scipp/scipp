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
    // slicing dimension, since it will have mismatching values.
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
