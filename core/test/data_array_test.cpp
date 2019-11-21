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

auto make_histogram() {
  auto edges = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, units::us,
                                    {0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>({Dim::X, 2}, {2.0, 3.0}, {0.3, 0.4});

  return DataArray(data, {{Dim::X, edges}});
}

TEST(DataArraySparseArithmeticTest, fail_sparse_op_non_histogram) {
  const auto sparse = make_sparse();
  auto coord = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {0, 2, 1, 3});
  auto data = makeVariable<double>({Dim::X, 2}, {2.0, 3.0}, {0.3, 0.4});
  DataArray not_hist(data, {{Dim::X, coord}});

  EXPECT_THROW(sparse * not_hist, except::SparseDataError);
  EXPECT_THROW(not_hist * sparse, except::SparseDataError);
  EXPECT_THROW(sparse / not_hist, except::SparseDataError);
}

TEST(DataArraySparseArithmeticTest, sparse_times_histogram) {
  const auto sparse = make_sparse();
  const auto hist = make_histogram();

  for (const auto result : {sparse * hist, hist * sparse}) {
    EXPECT_EQ(result.coords()[Dim::X], sparse.coords()[Dim::X]);
    EXPECT_TRUE(result.hasVariances());

    const auto out_vals = result.data().sparseValues<double>();
    const auto out_vars = result.data().sparseVariances<double>();

    auto expected =
        makeVariable<double>({Dim::X, 3}, {1, 1, 1}, {1, 1, 1}) *
        makeVariable<double>({Dim::X, 3}, {2.0, 3.0, 3.0}, {0.3, 0.4, 0.4});
    EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
    // out of range of edges -> value set to 0, consistent with rebin behavior
    expected = makeVariable<double>({Dim::X, 4}, {1, 1, 1, 1}, {1, 1, 1, 1}) *
               makeVariable<double>({Dim::X, 4}, {2.0, 2.0, 3.0, 0.0},
                                    {0.3, 0.3, 0.4, 0.0});
    EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[1], expected.variances<double>()));
  }
}

TEST(DataArraySparseArithmeticTest, sparse_with_values_times_histogram) {
  auto sparse = make_sparse();
  const auto hist = make_histogram();
  Variable data(sparse.coords()[Dim::X]);
  data.setUnit(units::counts);
  data *= 0.0;
  data += 2.0 * units::Unit(units::counts);
  sparse.setData(data);

  for (const auto result : {sparse * hist, hist * sparse}) {
    EXPECT_EQ(result.coords()[Dim::X], sparse.coords()[Dim::X]);
    EXPECT_TRUE(result.hasVariances());
    const auto out_vals = result.data().sparseValues<double>();
    const auto out_vars = result.data().sparseVariances<double>();

    auto expected =
        makeVariable<double>({Dim::X, 3}, {2, 2, 2}, {0, 0, 0}) *
        makeVariable<double>({Dim::X, 3}, {2.0, 3.0, 3.0}, {0.3, 0.4, 0.4});
    EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
    // out of range of edges -> value set to 0, consistent with rebin behavior
    expected = makeVariable<double>({Dim::X, 4}, {2, 2, 2, 2}, {0, 0, 0, 0}) *
               makeVariable<double>({Dim::X, 4}, {2.0, 2.0, 3.0, 0.0},
                                    {0.3, 0.3, 0.4, 0.0});
    EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
    EXPECT_TRUE(equals(out_vars[1], expected.variances<double>()));
  }
}

TEST(DataArraySparseArithmeticTest, sparse_over_histogram) {
  const auto sparse = make_sparse();
  const auto hist = make_histogram();

  const auto result = sparse / hist;
  EXPECT_EQ(result.coords()[Dim::X], sparse.coords()[Dim::X]);
  EXPECT_TRUE(result.hasVariances());
  const auto out_vals = result.data().sparseValues<double>();
  const auto out_vars = result.data().sparseVariances<double>();

  auto expected =
      makeVariable<double>({Dim::X, 3}, {1, 1, 1}, {1, 1, 1}) /
      makeVariable<double>({Dim::X, 3}, {2.0, 3.0, 3.0}, {0.3, 0.4, 0.4});
  EXPECT_TRUE(equals(out_vals[0], expected.values<double>()));
  EXPECT_TRUE(equals(out_vars[0], expected.variances<double>()));
  expected = makeVariable<double>({Dim::X, 4}, {1, 1, 1, 1}, {1, 1, 1, 1}) /
             makeVariable<double>({Dim::X, 4}, {2.0, 2.0, 3.0, 0.0},
                                  {0.3, 0.3, 0.4, 0.0});
  EXPECT_TRUE(equals(out_vals[1], expected.values<double>()));
  EXPECT_TRUE(equals(span<const double>(out_vars[1]).subspan(0, 3),
                     expected.slice({Dim::X, 0, 3}).variances<double>()));
  EXPECT_TRUE(std::isnan(out_vars[1][3]));
}
