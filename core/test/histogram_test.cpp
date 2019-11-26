// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"
#include "scipp/core/histogram.h"

using namespace scipp;
using namespace scipp::core;

TEST(HistogramTest, is_histogram) {
  const auto dataX = makeVariable<double>({Dim::X, 2});
  const auto dataY = makeVariable<double>({Dim::Y, 2});
  const auto dataXY = makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}});
  const auto edgesX = makeVariable<double>({Dim::X, 3});
  const auto edgesY = makeVariable<double>({Dim::Y, 4});
  const auto coordX = makeVariable<double>({Dim::X, 2});
  const auto coordY = makeVariable<double>({Dim::Y, 3});

  const auto histX = DataArray(dataX, {{Dim::X, edgesX}});
  EXPECT_TRUE(is_histogram(histX, Dim::X));
  EXPECT_FALSE(is_histogram(histX, Dim::Y));

  const auto histX2d = DataArray(dataXY, {{Dim::X, edgesX}});
  EXPECT_TRUE(is_histogram(histX2d, Dim::X));
  EXPECT_FALSE(is_histogram(histX2d, Dim::Y));

  const auto histY2d = DataArray(dataXY, {{Dim::X, coordX}, {Dim::Y, edgesY}});
  EXPECT_FALSE(is_histogram(histY2d, Dim::X));
  EXPECT_TRUE(is_histogram(histY2d, Dim::Y));

  EXPECT_FALSE(is_histogram(DataArray(dataX, {{Dim::X, coordX}}), Dim::X));
  EXPECT_FALSE(is_histogram(DataArray(dataX, {{Dim::X, coordY}}), Dim::X));
  EXPECT_FALSE(is_histogram(DataArray(dataX, {{Dim::Y, coordX}}), Dim::X));
  EXPECT_FALSE(is_histogram(DataArray(dataX, {{Dim::Y, coordY}}), Dim::X));

  // Coord length X is 2 and data does not depend on X, but this is *not*
  // interpreted as a single-bin histogram.
  EXPECT_FALSE(is_histogram(DataArray(dataY, {{Dim::X, coordX}}), Dim::X));

  const auto sparse = makeVariable<double>({Dim::X, Dimensions::Sparse});
  EXPECT_FALSE(is_histogram(DataArray(sparse, {{Dim::X, coordX}}), Dim::X));
}

Dataset make_2d_sparse_coord_only(const std::string &name) {
  Dataset sparse;
  auto var = makeVariable<double>({Dim::X, Dim::Y}, {3, Dimensions::Sparse});
  var.sparseValues<double>()[0] = {1.5, 2.5, 3.5, 4.5, 5.5};
  var.sparseValues<double>()[1] = {3.5, 4.5, 5.5, 6.5, 7.5};
  var.sparseValues<double>()[2] = {-1, 0, 0, 1, 1, 2, 2, 2, 4, 4, 4, 6};
  sparse.setSparseCoord(name, var);
  return sparse;
}

TEST(HistogramTest, fail_edges_not_sorted) {
  auto sparse = make_2d_sparse_coord_only("sparse");
  ASSERT_THROW(
      core::histogram(sparse["sparse"],
                      makeVariable<double>({Dim::Y, 6}, {1, 3, 2, 4, 5, 6})),
      except::BinEdgeError);
}

auto make_single_sparse() {
  Dataset sparse;
  sparse.setSparseCoord("sparse",
                        makeVariable<double>({Dim::X}, {Dimensions::Sparse}));
  sparse["sparse"].coords()[Dim::X].sparseValues<double>()[0] = {0, 1, 1, 2, 3};
  return sparse;
}

DataArray make_expected(const Variable &var, const Variable &edges) {
  auto dim = var.dims().inner();
  std::map<Dim, Variable> coords = {{dim, edges}};
  auto expected = DataArray(var, coords, {}, {}, {}, "sparse");
  return expected;
}

TEST(HistogramTest, below) {
  const auto sparse = make_single_sparse();
  auto edges = makeVariable<double>({Dim::X, 3}, {-2.0, -1.0, 0.0});
  auto hist = core::histogram(sparse["sparse"], edges);
  std::map<Dim, Variable> coords = {{Dim::X, edges}};
  auto expected = make_expected(
      makeVariable<double>({Dim::X, 2}, units::counts, {0, 0}, {0, 0}), edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, between) {
  const auto sparse = make_single_sparse();
  auto edges = makeVariable<double>({Dim::X, 3}, {1.5, 1.6, 1.7});
  auto hist = core::histogram(sparse["sparse"], edges);
  auto expected = make_expected(
      makeVariable<double>({Dim::X, 2}, units::counts, {0, 0}, {0, 0}), edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, above) {
  const auto sparse = make_single_sparse();
  auto edges = makeVariable<double>({Dim::X, 3}, {3.5, 4.5, 5.5});
  auto hist = core::histogram(sparse["sparse"], edges);
  auto expected = make_expected(
      makeVariable<double>({Dim::X, 2}, units::counts, {0, 0}, {0, 0}), edges);
  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, data_proxy) {
  auto sparse = make_2d_sparse_coord_only("sparse");
  std::vector<double> ref{1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 2, 3, 0, 3, 0};
  auto edges = makeVariable<double>({Dim::Y, 6}, {1, 2, 3, 4, 5, 6});
  auto hist = core::histogram(sparse["sparse"], edges);
  auto expected = make_expected(
      makeVariable<double>({{Dim::X, 3}, {Dim::Y, 5}}, units::counts, ref, ref),
      edges);

  EXPECT_EQ(hist, expected);
}

TEST(HistogramTest, with_data) {
  auto sparse = make_2d_sparse_coord_only("sparse");
  Variable data = makeVariableWithVariances<double>(
      {{Dim::X, 3}, {Dim::Y, Dimensions::Sparse}});
  data.sparseValues<double>()[0] = {1, 1, 1, 2, 2};
  data.sparseValues<double>()[1] = {2, 2, 2, 2, 2};
  data.sparseValues<double>()[2] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  data.sparseVariances<double>()[0] = {1, 1, 1, 2, 2};
  data.sparseVariances<double>()[1] = {2, 2, 2, 2, 2};
  data.sparseVariances<double>()[2] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  data.setUnit(units::counts);
  sparse.setData("sparse", data);
  auto edges = makeVariable<double>({Dim::Y, 6}, {1, 2, 3, 4, 5, 6});
  std::vector<double> ref{1, 1, 1, 2, 2, 0, 0, 2, 2, 2, 2, 3, 0, 3, 0};
  auto expected = make_expected(
      makeVariable<double>({{Dim::X, 3}, {Dim::Y, 5}}, units::counts, ref, ref),
      edges);

  EXPECT_EQ(core::histogram(sparse["sparse"], edges), expected);
}

TEST(HistogramTest, dataset) {
  auto sparse = make_2d_sparse_coord_only("a");
  sparse.setData("b", sparse["a"]);
  sparse["b"].coords()[Dim::Y] += 1.0;
  std::vector<double> a{1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 2, 3, 0, 3, 0};
  std::vector<double> b{0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 2, 2, 3, 0, 3};
  const auto coord = makeVariable<double>({Dim::Y, 6}, {1, 2, 3, 4, 5, 6});
  Dataset expected;
  expected.setCoord(Dim::Y, coord);
  expected.setData("a", makeVariable<double>({{Dim::X, 3}, {Dim::Y, 5}},
                                             units::counts, a, a));
  expected.setData("b", makeVariable<double>({{Dim::X, 3}, {Dim::Y, 5}},
                                             units::counts, b, b));

  EXPECT_EQ(core::histogram(sparse, coord), expected);
}

TEST(HistogramTest, dataset_own_coord) {
  auto sparse = make_2d_sparse_coord_only("a");
  sparse.setData("b", sparse["a"]);
  sparse["b"].coords()[Dim::Y] += 1.0;
  const auto coord = makeVariable<double>({Dim::Y, 6}, {1, 2, 3, 4, 5, 6});
  const auto expected = core::histogram(sparse, coord);

  sparse.setCoord(Dim::Y, coord);
  EXPECT_EQ(core::histogram(sparse, Dim::Y), expected);
}
