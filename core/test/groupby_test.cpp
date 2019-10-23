// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/groupby.h"

using namespace scipp;
using namespace scipp::core;

static auto make_dataset_for_groupby_test() {
  Dataset d;
  d.setData("a",
            makeVariable<int>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6}));
  d.setData("b", makeVariable<double>({Dim::X, 3}, units::s, {0.1, 0.2, 0.3}));
  d.setData("c", makeVariable<double>({{Dim::Z, 2}, {Dim::X, 3}}, units::s,
                                      {1, 2, 3, 4, 5, 6}));
  d.setAttr("scalar", makeVariable<double>(1.2));
  d.setLabels("labels1",
              makeVariable<double>({Dim::X, 3}, units::m, {1, 2, 3}));
  d.setLabels("labels2",
              makeVariable<double>({Dim::X, 3}, units::m, {1, 1, 3}));
  return d;
}

TEST(GroupbyTest, fail_key_not_found) {
  Dataset d = make_dataset_for_groupby_test();
  EXPECT_THROW(groupby(d, "invalid", Dim::Y), except::NotFoundError);
}

TEST(GroupbyTest, fail_key_2d) {
  Dataset d = make_dataset_for_groupby_test();
  d.setLabels("2d", makeVariable<double>({{Dim::Z, 2}, {Dim::X, 3}}, units::s,
                                         {1, 2, 3, 4, 5, 6}));
  EXPECT_THROW(groupby(d, "2d", Dim::Y), except::DimensionError);
}

TEST(GroupbyTest, fail_key_with_variances) {
  Dataset d = make_dataset_for_groupby_test();
  d.setLabels("variances",
              makeVariable<int>({Dim::X, 3}, units::m, {1, 2, 3}, {4, 5, 6}));
  EXPECT_THROW(groupby(d, "variances", Dim::Y), except::VariancesError);
}

TEST(GroupbyTest, dataset_1d_and_2d) {
  Dataset d = make_dataset_for_groupby_test();

  Dataset expected;
  expected.setData("a", makeVariable<double>({Dim::Y, 2}, units::m, {1.5, 3.0},
                                             {9.0 / 4, 6.0}));
  expected.setData("b", makeVariable<double>({Dim::Y, 2}, units::s,
                                             {(0.1 + 0.2) / 2.0, 0.3}));
  expected.setData("c", makeVariable<double>({{Dim::Z, 2}, {Dim::Y, 2}},
                                             units::s, {1.5, 3.0, 4.5, 6.0}));
  expected.setAttr("scalar", makeVariable<double>(1.2));
  expected.setCoord(Dim::Y,
                    makeVariable<double>({Dim::Y, 2}, units::m, {1, 3}));

  auto grouped = groupby(d, "labels2", Dim::Y);
  EXPECT_EQ(grouped.mean(Dim::X), expected);
}

static auto make_dataset_for_bin_test() {
  Dataset d;
  d.setData("a", makeVariable<double>({Dim::X, 5}, units::s,
                                      {0.1, 0.2, 0.3, 0.4, 0.5}));
  d.setData("b", makeVariable<double>({{Dim::Y, 2}, {Dim::X, 5}}, units::s,
                                      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
  d.setAttr("scalar", makeVariable<double>(1.2));
  d.setLabels("labels1",
              makeVariable<double>({Dim::X, 5}, units::m, {1, 2, 3, 4, 5}));
  d.setLabels("labels2", makeVariable<double>({Dim::X, 5}, units::m,
                                              {1.0, 1.1, 2.5, 4.0, 1.2}));
  return d;
}

TEST(GroupbyTest, bins) {
  Dataset d = make_dataset_for_bin_test();

  auto bins = makeVariable<double>({Dim::Z, 4}, units::m, {0.0, 1.0, 2.0, 3.0});

  Dataset expected;
  expected.setCoord(Dim::Z, bins);
  expected.setData(
      "a", makeVariable<double>({Dim::Z, 3}, units::s, {0.0, 0.8, 0.3}));
  expected.setData("b", makeVariable<double>({{Dim::Y, 2}, {Dim::Z, 3}},
                                             units::s, {0, 8, 3, 0, 23, 8}));
  expected.setAttr("scalar", makeVariable<double>(1.2));

  EXPECT_EQ(groupby(d, "labels2", bins).sum(Dim::X), expected);
}

TEST(GroupbyTest, bins_mean_empty) {
  Dataset d = make_dataset_for_bin_test();

  auto bins = makeVariable<double>({Dim::Z, 4}, units::m, {0.0, 1.0, 2.0, 3.0});

  const auto binned = groupby(d, "labels2", bins).mean(Dim::X);
  EXPECT_TRUE(std::isnan(binned["a"].values<double>()[0]));
  EXPECT_FALSE(std::isnan(binned["a"].values<double>()[1]));
  EXPECT_TRUE(std::isnan(binned["b"].values<double>()[0]));
  EXPECT_TRUE(std::isnan(binned["b"].values<double>()[3]));
  EXPECT_FALSE(std::isnan(binned["b"].values<double>()[1]));
}

TEST(GroupbyTest, single_bin) {
  Dataset d = make_dataset_for_bin_test();

  auto bins = makeVariable<double>({Dim::Z, 2}, units::m, {1.0, 5.0});
  const auto groups = groupby(d, "labels2", bins);

  // Non-range slice drops Dim::Z and the corresponding coord (the edges), so
  // the result must be equal to a global `sum` or `mean`.
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 0}), sum(d, Dim::X));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 0}), mean(d, Dim::X));
}

TEST(GroupbyTest, two_bin) {
  Dataset d = make_dataset_for_bin_test();

  auto bins = makeVariable<double>({Dim::Z, 3}, units::m, {1.0, 2.0, 5.0});
  const auto groups = groupby(d, "labels2", bins);

  auto group0 =
      concatenate(d.slice({Dim::X, 0, 2}), d.slice({Dim::X, 4, 5}), Dim::X);
  // concatenate does currently not preserve attributes
  group0.setAttr("scalar", d.attrs()["scalar"]);
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 0}), sum(group0, Dim::X));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 0}), mean(group0, Dim::X));

  const auto group1 = d.slice({Dim::X, 2, 4});
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 1}), sum(group1, Dim::X));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 1}), mean(group1, Dim::X));
}
