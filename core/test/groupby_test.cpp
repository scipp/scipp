// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/groupby.h"

#include "test_macros.h"

using namespace scipp;
using namespace scipp::core;

static auto make_dataset_for_groupby_test() {
  Dataset d;
  d.setData("a",
            createVariable<int>(Dims{Dim::X}, Shape{3}, units::Unit(units::m),
                                Values{1, 2, 3}, Variances{4, 5, 6}));
  d.setData("b", createVariable<double>(Dims{Dim::X}, Shape{3},
                                        units::Unit(units::s),
                                        Values{0.1, 0.2, 0.3}));
  d.setData("c", createVariable<double>(Dims{Dim::Z, Dim::X}, Shape{2, 3},
                                        units::Unit(units::s),
                                        Values{1, 2, 3, 4, 5, 6}));
  d.setAttr("a", "scalar", createVariable<double>(Values{1.2}));
  d.setLabels("labels1",
              createVariable<double>(Dims{Dim::X}, Shape{3},
                                     units::Unit(units::m), Values{1, 2, 3}));
  d.setLabels("labels2",
              createVariable<double>(Dims{Dim::X}, Shape{3},
                                     units::Unit(units::m), Values{1, 1, 3}));
  return d;
}

TEST(GroupbyTest, fail_key_not_found) {
  Dataset d = make_dataset_for_groupby_test();
  EXPECT_THROW(groupby(d, "invalid", Dim::Y), except::NotFoundError);
  EXPECT_THROW(groupby(d["a"], "invalid", Dim::Y), except::NotFoundError);
}

TEST(GroupbyTest, fail_key_2d) {
  Dataset d = make_dataset_for_groupby_test();
  d.setLabels("2d", createVariable<double>(Dims{Dim::Z, Dim::X}, Shape{2, 3},
                                           units::Unit(units::s),
                                           Values{1, 2, 3, 4, 5, 6}));
  EXPECT_THROW(groupby(d, "2d", Dim::Y), except::DimensionError);
  EXPECT_THROW(groupby(d["a"], "2d", Dim::Y), except::DimensionError);
}

TEST(GroupbyTest, fail_key_with_variances) {
  Dataset d = make_dataset_for_groupby_test();
  d.setLabels("variances",
              createVariable<int>(Dims{Dim::X}, Shape{3}, units::Unit(units::m),
                                  Values{1, 2, 3}, Variances{4, 5, 6}));
  EXPECT_THROW(groupby(d, "variances", Dim::Y), except::VariancesError);
  EXPECT_THROW(groupby(d["a"], "variances", Dim::Y), except::VariancesError);
}

TEST(GroupbyTest, dataset_1d_and_2d) {
  Dataset d = make_dataset_for_groupby_test();

  Dataset expected;
  expected.setData(
      "a", createVariable<double>(Dims{Dim::Y}, Shape{2}, units::Unit(units::m),
                                  Values{1.5, 3.0}, Variances{9.0 / 4, 6.0}));
  expected.setData("b", createVariable<double>(Dims{Dim::Y}, Shape{2},
                                               units::Unit(units::s),
                                               Values{(0.1 + 0.2) / 2.0, 0.3}));
  expected.setData("c", createVariable<double>(
                            Dims{Dim::Z, Dim::Y}, Shape{2, 2},
                            units::Unit(units::s), Values{1.5, 3.0, 4.5, 6.0}));
  expected.setAttr("a", "scalar", createVariable<double>(Values{1.2}));
  expected.setCoord(Dim::Y, createVariable<double>(Dims{Dim::Y}, Shape{2},
                                                   units::Unit(units::m),
                                                   Values{1, 3}));

  EXPECT_EQ(groupby(d, "labels2", Dim::Y).mean(Dim::X), expected);
  EXPECT_EQ(groupby(d["a"], "labels2", Dim::Y).mean(Dim::X), expected["a"]);
  EXPECT_EQ(groupby(d["b"], "labels2", Dim::Y).mean(Dim::X), expected["b"]);
  EXPECT_EQ(groupby(d["c"], "labels2", Dim::Y).mean(Dim::X), expected["c"]);
}

static auto make_dataset_for_bin_test() {
  Dataset d;
  d.setData("a", createVariable<double>(Dims{Dim::X}, Shape{5},
                                        units::Unit(units::s),
                                        Values{0.1, 0.2, 0.3, 0.4, 0.5}));
  d.setData("b", createVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 5},
                                        units::Unit(units::s),
                                        Values{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}));
  d.setAttr("a", "scalar", createVariable<double>(Values{1.2}));
  d.setLabels("labels1", createVariable<double>(Dims{Dim::X}, Shape{5},
                                                units::Unit(units::m),
                                                Values{1, 2, 3, 4, 5}));
  d.setLabels("labels2", createVariable<double>(
                             Dims{Dim::X}, Shape{5}, units::Unit(units::m),
                             Values{1.0, 1.1, 2.5, 4.0, 1.2}));
  return d;
}

TEST(GroupbyTest, bins) {
  Dataset d = make_dataset_for_bin_test();

  auto bins =
      createVariable<double>(Dims{Dim::Z}, Shape{4}, units::Unit(units::m),
                             Values{0.0, 1.0, 2.0, 3.0});

  Dataset expected;
  expected.setCoord(Dim::Z, bins);
  expected.setData("a", createVariable<double>(Dims{Dim::Z}, Shape{3},
                                               units::Unit(units::s),
                                               Values{0.0, 0.8, 0.3}));
  expected.setData("b", createVariable<double>(
                            Dims{Dim::Y, Dim::Z}, Shape{2, 3},
                            units::Unit(units::s), Values{0, 8, 3, 0, 23, 8}));
  expected.setAttr("a", "scalar", createVariable<double>(Values{1.2}));

  EXPECT_EQ(groupby(d, "labels2", bins).sum(Dim::X), expected);
  EXPECT_EQ(groupby(d["a"], "labels2", bins).sum(Dim::X), expected["a"]);
  EXPECT_EQ(groupby(d["b"], "labels2", bins).sum(Dim::X), expected["b"]);
}

TEST(GroupbyTest, bins_mean_empty) {
  Dataset d = make_dataset_for_bin_test();

  auto bins =
      createVariable<double>(Dims{Dim::Z}, Shape{4}, units::Unit(units::m),
                             Values{0.0, 1.0, 2.0, 3.0});

  const auto binned = groupby(d, "labels2", bins).mean(Dim::X);
  EXPECT_TRUE(std::isnan(binned["a"].values<double>()[0]));
  EXPECT_FALSE(std::isnan(binned["a"].values<double>()[1]));
  EXPECT_TRUE(std::isnan(binned["b"].values<double>()[0]));
  EXPECT_TRUE(std::isnan(binned["b"].values<double>()[3]));
  EXPECT_FALSE(std::isnan(binned["b"].values<double>()[1]));
}

TEST(GroupbyTest, single_bin) {
  Dataset d = make_dataset_for_bin_test();

  auto bins = createVariable<double>(Dims{Dim::Z}, Shape{2},
                                     units::Unit(units::m), Values{1.0, 5.0});
  const auto groups = groupby(d, "labels2", bins);

  // Non-range slice drops Dim::Z and the corresponding coord (the edges), so
  // the result must be equal to a global `sum` or `mean`.
  EXPECT_EQ(groups.sum(Dim::X).slice({Dim::Z, 0}), sum(d, Dim::X));
  EXPECT_EQ(groups.mean(Dim::X).slice({Dim::Z, 0}), mean(d, Dim::X));
}

TEST(GroupbyTest, two_bin) {
  Dataset d = make_dataset_for_bin_test();

  auto bins = createVariable<double>(
      Dims{Dim::Z}, Shape{3}, units::Unit(units::m), Values{1.0, 2.0, 5.0});
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
  auto var = createVariable<double>(Dims{Dim::Y, Dim::X},
                                    Shape{3l, Dimensions::Sparse});
  const auto &var_ = var.sparseValues<double>();
  var_[0] = {1, 2, 3};
  var_[1] = {4, 5};
  var_[2] = {6, 7};
  return var;
}

auto make_sparse_out() {
  auto var = createVariable<double>(Dims{Dim::Z, Dim::X},
                                    Shape{2l, Dimensions::Sparse});
  const auto &var_ = var.sparseValues<double>();
  var_[0] = {1, 2, 3, 4, 5};
  var_[1] = {6, 7};
  return var;
}

DataArray make_sparse_array_in() {
  return {std::nullopt,
          {{Dim::X, make_sparse_in()}},
          {{"labels",
            createVariable<double>(Dims{Dim::Y}, Shape{3},
                                   units::Unit(units::m), Values{1, 1, 3})},
           {"dense", createVariable<double>(Dims{Dim::X}, Shape{5},
                                            units::Unit(units::m),
                                            Values{1, 2, 3, 4, 5})}},
          {},
          {{"scalar_attr", createVariable<double>(Values{1.2})}}};
}

DataArray make_sparse_array_out() {
  return {
      std::nullopt,
      {{Dim::X, make_sparse_out()},
       {Dim::Z, createVariable<double>(Dims{Dim::Z}, Shape{2},
                                       units::Unit(units::m), Values{1, 3})}},
      {{"dense",
        createVariable<double>(Dims{Dim::X}, Shape{5}, units::Unit(units::m),
                               Values{1, 2, 3, 4, 5})}},
      {},
      {{"scalar_attr", createVariable<double>(Values{1.2})}}};
}

TEST(GroupbyTest, flatten_coord_only) {
  const auto a = make_sparse_array_in();
  const auto expected = make_sparse_array_out();

  EXPECT_EQ(groupby(a, "labels", Dim::Z).flatten(Dim::Y), expected);
}

TEST(GroupbyTest, flatten_coord_and_labels) {
  DataArray a{std::nullopt,
              {{Dim::X, make_sparse_in()}},
              {{"sparse", make_sparse_in() * 0.3},
               {"labels", createVariable<double>(Dims{Dim::Y}, Shape{3},
                                                 units::Unit(units::m),
                                                 Values{1, 1, 3})}}};

  DataArray expected{
      std::nullopt,
      {{Dim::X, make_sparse_out()},
       {Dim::Z, createVariable<double>(Dims{Dim::Z}, Shape{2},
                                       units::Unit(units::m), Values{1, 3})}},
      {{"sparse", make_sparse_out() * 0.3}}};

  EXPECT_EQ(groupby(a, "labels", Dim::Z).flatten(Dim::Y), expected);
}

TEST(GroupbyTest, flatten_coord_and_data) {
  DataArray a{make_sparse_in() * 1.5,
              {{Dim::X, make_sparse_in()}},
              {{"labels", createVariable<double>(Dims{Dim::Y}, Shape{3},
                                                 units::Unit(units::m),
                                                 Values{1, 1, 3})}}};

  DataArray expected{
      make_sparse_out() * 1.5,
      {{Dim::X, make_sparse_out()},
       {Dim::Z, createVariable<double>(Dims{Dim::Z}, Shape{2},
                                       units::Unit(units::m), Values{1, 3})}}};

  EXPECT_EQ(groupby(a, "labels", Dim::Z).flatten(Dim::Y), expected);
}

TEST(GroupbyTest, flatten_dataset_coord_only) {
  const auto a = make_sparse_array_in();
  const auto expected = make_sparse_array_out();

  const Dataset d{{{"a", a}, {"b", a}}};
  const Dataset expected_d{{{"a", expected}, {"b", expected}}};
  EXPECT_EQ(groupby(d, "labels", Dim::Z).flatten(Dim::Y), expected_d);
}
