// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

#include "dataset_test_common.h"
#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

TEST(DataArrayTest, construct) {
  DatasetFactory3D factory;
  const auto dataset = factory.make();

  DataArray array(dataset["data_xyz"]);
  EXPECT_EQ(array, dataset["data_xyz"]);
  // Comparison ignores the name, so this is tested separately.
  EXPECT_EQ(array.name(), "data_xyz");
}

TEST(DataArrayTest, sum_dataset_columns_via_DataArray) {
  DatasetFactory3D factory;
  auto dataset = factory.make();

  DataArray array(dataset["data_zyx"]);
  auto sum = array + dataset["data_xyz"];

  dataset["data_zyx"] += dataset["data_xyz"];

  // This would fail if the data items had attributes, since += preserves them
  // but + does not.
  EXPECT_EQ(sum, dataset["data_zyx"]);
}

auto make_sparse() {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  var.setUnit(units::us);
  auto vals = var.sparseValues<double>();
  vals[0] = {1.1, 2.2, 3.3};
  vals[1] = {1.1, 2.2, 3.3, 5.5};
  return DataArray(std::optional<Variable>(), {{Dim::X, var}});
}

TEST(DataArraySparseArithmeticTest, fail_sparse_with_non_histogram) {
  const auto sparse = make_sparse();
  auto coord = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {0, 2, 1, 3});
  auto weights = makeVariable<double>({Dim::X, 2}, {2.0, 3.0}, {0.3, 0.4});
  DataArray not_hist(weights, {{Dim::X, coord}});

  EXPECT_THROW(sparse * not_hist, except::SparseDataError);
}

TEST(DataArraySparseArithmeticTest, sparse_times_histogram) {
  const auto sparse = make_sparse();
  auto edges = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, units::us,
                                    {0, 2, 4, 1, 3, 5});
  auto weights = makeVariable<double>({Dim::X, 2}, {2.0, 3.0}, {0.3, 0.4});

  DataArray hist(weights, {{Dim::X, edges}});

  for (const auto result : {sparse * hist, hist * sparse}) {
    EXPECT_EQ(result.coords()[Dim::X], sparse.coords()[Dim::X]);
    EXPECT_TRUE(result.hasVariances());
    const auto out_vals = result.data().sparseValues<double>();
    EXPECT_TRUE(equals(out_vals[0], {2, 3, 3}));
    // out of range of edges -> value set to 0, consistent with rebin behavior
    EXPECT_TRUE(equals(out_vals[1], {2, 2, 3, 0}));
    const auto out_vars = result.data().sparseVariances<double>();
    EXPECT_TRUE(equals(out_vars[0], {0.3, 0.4, 0.4}));
    EXPECT_TRUE(equals(out_vars[1], {0.3, 0.3, 0.4, 0.0}));
  }
}

TEST(DataArraySparseArithmeticTest, sparse_over_histogram) {
  const auto sparse = make_sparse();
  auto edges = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, units::us,
                                    {0, 2, 4, 1, 3, 5});
  auto weights = makeVariable<double>({Dim::X, 2}, {2.0, 3.0}, {0.3, 0.4});

  DataArray hist(weights, {{Dim::X, edges}});

  const auto result = sparse / hist;
  EXPECT_EQ(result.coords()[Dim::X], sparse.coords()[Dim::X]);
  EXPECT_TRUE(result.hasVariances());
  const auto out_vals = result.data().sparseValues<double>();
  EXPECT_TRUE(equals(out_vals[0], {1.0 / 2, 1.0 / 3, 1.0 / 3}));
  EXPECT_TRUE(equals(out_vals[1], {1.0 / 2, 1.0 / 2, 1.0 / 3,
                                   std::numeric_limits<double>::infinity()}));
  const auto out_vars = result.data().sparseVariances<double>();
  EXPECT_TRUE(equals(out_vars[0], {0.3 / (2 * 2 * 2 * 2), 0.4 / (3 * 3 * 3 * 3),
                                   0.4 / (3 * 3 * 3 * 3)}));
  EXPECT_EQ(out_vars[1][0], 0.3 / (2 * 2 * 2 * 2));
  EXPECT_EQ(out_vars[1][1], 0.3 / (2 * 2 * 2 * 2));
  EXPECT_EQ(out_vars[1][2], 0.4 / (3 * 3 * 3 * 3));
  EXPECT_TRUE(std::isnan(out_vars[1][3]));
}
