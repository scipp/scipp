// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/groupby.h"

#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

struct GroupbyTest : public ::testing::Test {
  GroupbyTest() {
    d.setData("a",
              makeVariable<int>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6}));
    d.setData("b",
              makeVariable<double>({Dim::X, 3}, units::s, {0.1, 0.2, 0.3}));
    d.setData("c", makeVariable<double>({{Dim::Z, 2}, {Dim::X, 3}}, units::s,
                                        {1, 2, 3, 4, 5, 6}));
    d.setAttr("a", "scalar", makeVariable<double>(1.2));
    d.setLabels("labels1",
                makeVariable<double>({Dim::X, 3}, units::m, {1, 2, 3}));
    d.setLabels("labels2",
                makeVariable<double>({Dim::X, 3}, units::m, {1, 1, 3}));
  }

  Dataset d;
};

TEST_F(GroupbyTest, fail_key_not_found) {
  EXPECT_THROW(groupby(d, "invalid", Dim::Y), except::NotFoundError);
  EXPECT_THROW(groupby(d["a"], "invalid", Dim::Y), except::NotFoundError);
}

TEST_F(GroupbyTest, fail_key_2d) {
  d.setLabels("2d", makeVariable<double>({{Dim::Z, 2}, {Dim::X, 3}}, units::s,
                                         {1, 2, 3, 4, 5, 6}));
  EXPECT_THROW(groupby(d, "2d", Dim::Y), except::DimensionError);
  EXPECT_THROW(groupby(d["a"], "2d", Dim::Y), except::DimensionError);
}

TEST_F(GroupbyTest, fail_key_with_variances) {
  d.setLabels("variances",
              makeVariable<int>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6}));
  EXPECT_THROW(groupby(d, "variances", Dim::Y), except::VariancesError);
  EXPECT_THROW(groupby(d["a"], "variances", Dim::Y), except::VariancesError);
}

TEST_F(GroupbyTest, dataset_1d_and_2d) {

  Dataset expected;
  expected.setData("a", makeVariable<double>({Dim::Y, 2}, units::m, {1.5, 3.0},
                                             {9.0 / 4, 6.0}));
  expected.setData("b", makeVariable<double>({Dim::Y, 2}, units::s,
                                             {(0.1 + 0.2) / 2.0, 0.3}));
  expected.setData("c", makeVariable<double>({{Dim::Z, 2}, {Dim::Y, 2}},
                                             units::s, {1.5, 3.0, 4.5, 6.0}));
  expected.setAttr("a", "scalar", makeVariable<double>(1.2));
  expected.setCoord(Dim::Y,
                    makeVariable<double>({Dim::Y, 2}, units::m, {1, 3}));

  EXPECT_EQ(groupby(d, "labels2", Dim::Y).mean(Dim::X), expected);
  EXPECT_EQ(groupby(d["a"], "labels2", Dim::Y).mean(Dim::X), expected["a"]);
  EXPECT_EQ(groupby(d["b"], "labels2", Dim::Y).mean(Dim::X), expected["b"]);
  EXPECT_EQ(groupby(d["c"], "labels2", Dim::Y).mean(Dim::X), expected["c"]);
}

struct GroupbyWithBinsTest : public ::testing::Test {
  GroupbyWithBinsTest() {
    d.setData("a", makeVariable<double>({Dim::X, 5}, units::s,
                                        {0.1, 0.2, 0.3, 0.4, 0.5}));
    d.setData("b", makeVariable<double>({{Dim::Y, 2}, {Dim::X, 5}}, units::s,
                                        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
    d.setAttr("a", "scalar", makeVariable<double>(1.2));
    d.setLabels("labels1",
                makeVariable<double>({Dim::X, 5}, units::m, {1, 2, 3, 4, 5}));
    d.setLabels("labels2", makeVariable<double>({Dim::X, 5}, units::m,
                                                {1.0, 1.1, 2.5, 4.0, 1.2}));
  }

  Dataset d;
};

TEST_F(GroupbyWithBinsTest, bins) {
  auto bins = makeVariable<double>({Dim::Z, 4}, units::m, {0.0, 1.0, 2.0, 3.0});

  Dataset expected;
  expected.setCoord(Dim::Z, bins);
  expected.setData(
      "a", makeVariable<double>({Dim::Z, 3}, units::s, {0.0, 0.8, 0.3}));
  expected.setData("b", makeVariable<double>({{Dim::Y, 2}, {Dim::Z, 3}},
                                             units::s, {0, 8, 3, 0, 23, 8}));
  expected.setAttr("a", "scalar", makeVariable<double>(1.2));

  EXPECT_EQ(groupby(d, "labels2", bins).sum(Dim::X), expected);
  EXPECT_EQ(groupby(d["a"], "labels2", bins).sum(Dim::X), expected["a"]);
  EXPECT_EQ(groupby(d["b"], "labels2", bins).sum(Dim::X), expected["b"]);
}

TEST_F(GroupbyWithBinsTest, bins_mean_empty) {
  auto bins = makeVariable<double>({Dim::Z, 4}, units::m, {0.0, 1.0, 2.0, 3.0});

  const auto binned = groupby(d, "labels2", bins).mean(Dim::X);
  EXPECT_TRUE(std::isnan(binned["a"].values<double>()[0]));
  EXPECT_FALSE(std::isnan(binned["a"].values<double>()[1]));
  EXPECT_TRUE(std::isnan(binned["b"].values<double>()[0]));
  EXPECT_TRUE(std::isnan(binned["b"].values<double>()[3]));
  EXPECT_FALSE(std::isnan(binned["b"].values<double>()[1]));
}

TEST_F(GroupbyWithBinsTest, single_bin) {
  auto bins = makeVariable<double>({Dim::Z, 2}, units::m, {1.0, 5.0});
  const auto groups = groupby(d, "labels2", bins);

  // Non-range slice drops Dim::Z and the corresponding coord (the edges), so
  // the result must be equal to a global `sum` or `mean`.
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 0}), sum(d, Dim::X));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 0}), mean(d, Dim::X));
}

TEST_F(GroupbyWithBinsTest, two_bin) {
  auto bins = makeVariable<double>({Dim::Z, 3}, units::m, {1.0, 2.0, 5.0});
  const auto groups = groupby(d, "labels2", bins);

  auto group0 =
      concatenate(d.slice({Dim::X, 0, 2}), d.slice({Dim::X, 4, 5}), Dim::X);
  // concatenate does currently not preserve attributes
  group0.setAttr("a", "scalar", d["a"].attrs()["scalar"]);
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 0}), sum(group0, Dim::X));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 0}), mean(group0, Dim::X));

  const auto group1 = d.slice({Dim::X, 2, 4});
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 1}), sum(group1, Dim::X));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 1}), mean(group1, Dim::X));
}

auto make_sparse_in() {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {3, Dimensions::Sparse});
  const auto &var_ = var.sparseValues<double>();
  var_[0] = {1, 2, 3};
  var_[1] = {4, 5};
  var_[2] = {6, 7};
  return var;
}

auto make_sparse_out() {
  auto var = makeVariable<double>({Dim::Z, Dim::X}, {2, Dimensions::Sparse});
  const auto &var_ = var.sparseValues<double>();
  var_[0] = {1, 2, 3, 4, 5};
  var_[1] = {6, 7};
  return var;
}

struct GroupbyFlattenCoordOnly : public ::testing::Test {
  const DataArray a{
      std::nullopt,
      {{Dim::X, make_sparse_in()}},
      {{"labels", makeVariable<double>({Dim::Y, 3}, units::m, {1, 1, 3})},
       {"dense", makeVariable<double>({Dim::X, 5}, units::m, {1, 2, 3, 4, 5})}},
      {},
      {{"scalar_attr", makeVariable<double>(1.2)}}};

  const DataArray expected{
      std::nullopt,
      {{Dim::X, make_sparse_out()},
       {Dim::Z, makeVariable<double>({Dim::Z, 2}, units::m, {1, 3})}},
      {{"dense", makeVariable<double>({Dim::X, 5}, units::m, {1, 2, 3, 4, 5})}},
      {},
      {{"scalar_attr", makeVariable<double>(1.2)}}};
};

TEST_F(GroupbyFlattenCoordOnly, flatten_coord_only) {
  EXPECT_EQ(groupby(a, "labels", Dim::Z).flatten(Dim::Y), expected);
}

TEST_F(GroupbyFlattenCoordOnly, flatten_dataset_coord_only) {
  const Dataset d{{{"a", a}, {"b", a}}};
  const Dataset expected_d{{{"a", expected}, {"b", expected}}};
  EXPECT_EQ(groupby(d, "labels", Dim::Z).flatten(Dim::Y), expected_d);
}

TEST(GroupbyFlattenTest, flatten_coord_and_labels) {
  DataArray a{
      std::nullopt,
      {{Dim::X, make_sparse_in()}},
      {{"sparse", make_sparse_in() * 0.3},
       {"labels", makeVariable<double>({Dim::Y, 3}, units::m, {1, 1, 3})}}};

  DataArray expected{
      std::nullopt,
      {{Dim::X, make_sparse_out()},
       {Dim::Z, makeVariable<double>({Dim::Z, 2}, units::m, {1, 3})}},
      {{"sparse", make_sparse_out() * 0.3}}};

  EXPECT_EQ(groupby(a, "labels", Dim::Z).flatten(Dim::Y), expected);
}

TEST(GroupbyFlattenTest, flatten_coord_and_data) {
  DataArray a{
      make_sparse_in() * 1.5,
      {{Dim::X, make_sparse_in()}},
      {{"labels", makeVariable<double>({Dim::Y, 3}, units::m, {1, 1, 3})}}};

  DataArray expected{
      make_sparse_out() * 1.5,
      {{Dim::X, make_sparse_out()},
       {Dim::Z, makeVariable<double>({Dim::Z, 2}, units::m, {1, 3})}}};

  EXPECT_EQ(groupby(a, "labels", Dim::Z).flatten(Dim::Y), expected);
}
