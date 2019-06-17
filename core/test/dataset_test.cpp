// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>

#include "dataset.h"
#include "dimensions.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

TEST(DatasetTest, construct_default) { ASSERT_NO_THROW(Dataset d); }

TEST(DatasetTest, empty) {
  Dataset d;
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.size(), 0);
}

TEST(DatasetTest, coords) {
  Dataset d;
  ASSERT_NO_THROW(d.coords());
}

TEST(DatasetTest, labels) {
  Dataset d;
  ASSERT_NO_THROW(d.labels());
}

TEST(DatasetTest, attrs) {
  Dataset d;
  ASSERT_NO_THROW(d.attrs());
}

TEST(DatasetTest, bad_item_access) {
  Dataset d;
  ASSERT_ANY_THROW(d[""]);
  ASSERT_ANY_THROW(d["abc"]);
}

TEST(DatasetTest, setCoord) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 0);

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 1);

  ASSERT_NO_THROW(d.setCoord(Dim::Y, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 2);

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.coords().size(), 2);
}

TEST(DatasetTest, setLabels) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 0);

  ASSERT_NO_THROW(d.setLabels("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 1);

  ASSERT_NO_THROW(d.setLabels("b", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 2);

  ASSERT_NO_THROW(d.setLabels("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.labels().size(), 2);
}

TEST(DatasetTest, setAttr) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 0);

  ASSERT_NO_THROW(d.setAttr("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 1);

  ASSERT_NO_THROW(d.setAttr("b", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 2);

  ASSERT_NO_THROW(d.setAttr("a", var));
  ASSERT_EQ(d.size(), 0);
  ASSERT_EQ(d.attrs().size(), 2);
}

TEST(DatasetTest, setData_with_and_without_variances) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_NO_THROW(d.setData("a", var));
  ASSERT_EQ(d.size(), 1);

  ASSERT_NO_THROW(d.setData("b", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setData("a", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(
      d.setData("a", makeVariable<double>({Dim::X, 3}, {1, 1, 1}, {0, 0, 0})));
  ASSERT_EQ(d.size(), 2);
}

TEST(DatasetTest, setData_dense_when_dimensions_sparse) {}

TEST(DatasetTest, setLabels_with_name_matching_data_name) {
  Dataset d;
  d.setData("a", makeVariable<double>({Dim::X, 3}));
  d.setData("b", makeVariable<double>({Dim::X, 3}));

  // It is possible to set labels with a name matching data. However, there is
  // no special meaning attached to this. In particular it is *not* linking the
  // labels to that data item.
  ASSERT_NO_THROW(d.setLabels("a", makeVariable<double>({})));
  ASSERT_EQ(d.size(), 2);
  ASSERT_EQ(d.labels().size(), 1);
  ASSERT_EQ(d["a"].labels().size(), 1);
  ASSERT_EQ(d["b"].labels().size(), 1);
}

TEST(DatasetTest, setSparseCoord_not_sparse_fail) {
  Dataset d;
  const auto var = makeVariable<double>({Dim::X, 3});

  ASSERT_ANY_THROW(d.setSparseCoord("a", var));
}

TEST(DatasetTest, setSparseCoord) {
  Dataset d;
  const auto var =
      makeVariable<double>({Dim::X, Dim::Y}, {3, Dimensions::Sparse});

  ASSERT_NO_THROW(d.setSparseCoord("a", var));
  ASSERT_EQ(d.size(), 1);
  ASSERT_NO_THROW(d["a"]);
}

TEST(DatasetTest, setSparseLabels_missing_values_or_coord) {
  Dataset d;
  const auto sparse = makeVariable<double>({Dim::X}, {Dimensions::Sparse});

  ASSERT_ANY_THROW(d.setSparseLabels("a", "x", sparse));
  d.setSparseCoord("a", sparse);
  ASSERT_NO_THROW(d.setSparseLabels("a", "x", sparse));
}

TEST(DatasetTest, setSparseLabels_not_sparse_fail) {
  Dataset d;
  const auto dense = makeVariable<double>({});
  const auto sparse = makeVariable<double>({Dim::X}, {Dimensions::Sparse});

  d.setSparseCoord("a", sparse);
  ASSERT_ANY_THROW(d.setSparseLabels("a", "x", dense));
}

TEST(DatasetTest, setSparseLabels) {
  Dataset d;
  const auto sparse = makeVariable<double>({Dim::X}, {Dimensions::Sparse});
  d.setSparseCoord("a", sparse);

  ASSERT_NO_THROW(d.setSparseLabels("a", "x", sparse));
  ASSERT_EQ(d.size(), 1);
  ASSERT_NO_THROW(d["a"]);
  ASSERT_EQ(d["a"].labels().size(), 1);
}

TEST(DatasetTest, iterators_empty_dataset) {
  Dataset d;
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetTest, iterators_only_coords) {
  Dataset d;
  d.setCoord(Dim::X, makeVariable<double>({}));
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetTest, iterators_only_labels) {
  Dataset d;
  d.setLabels("a", makeVariable<double>({}));
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetTest, iterators_only_attrs) {
  Dataset d;
  d.setAttr("a", makeVariable<double>({}));
  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());
  EXPECT_EQ(d.begin(), d.end());
}

TEST(DatasetTest, iterators) {
  Dataset d;
  d.setData("a", makeVariable<double>({}));
  d.setData("b", makeVariable<float>({}));
  d.setData("c", makeVariable<int64_t>({}));

  ASSERT_NO_THROW(d.begin());
  ASSERT_NO_THROW(d.end());

  auto it = d.begin();
  ASSERT_NE(it, d.end());
  EXPECT_EQ(it->first, "a");

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, d.end());
  EXPECT_EQ(it->first, "b");

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, d.end());
  EXPECT_EQ(it->first, "c");

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, d.end());
}

TEST(DatasetTest, iterators_return_types) {
  Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(d.begin()->second), DataProxy>));
  ASSERT_TRUE((std::is_same_v<decltype(d.end()->second), DataProxy>));
}

TEST(DatasetTest, const_iterators_return_types) {
  const Dataset d;
  ASSERT_TRUE((std::is_same_v<decltype(d.begin()->second), DataConstProxy>));
  ASSERT_TRUE((std::is_same_v<decltype(d.end()->second), DataConstProxy>));
}

TEST(DatasetTest, set_dense_data_with_sparse_coord) {

  auto sparse_variable =
      makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  auto dense_variable = makeVariable<double>({Dim::Y, Dim::X}, {2, 2});

  Dataset a;
  a.setData("sparse_coord_and_val", dense_variable);
  ASSERT_THROW(a.setSparseCoord("sparse_coord_and_val", sparse_variable),
               std::runtime_error);

  // Characterise temporal coupling issue below.
  Dataset b;
  b.setSparseCoord("sparse_coord_and_val", sparse_variable);
  b.setData("sparse_coord_and_val", dense_variable);
}

class Dataset_comparison_operators : public ::testing::Test {
private:
  template <class A, class B>
  void expect_eq_impl(const A &a, const B &b) const {
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(b == a);
    EXPECT_FALSE(a != b);
    EXPECT_FALSE(b != a);
  }
  template <class A, class B>
  void expect_ne_impl(const A &a, const B &b) const {
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != a);
    EXPECT_FALSE(a == b);
    EXPECT_FALSE(b == a);
  }

protected:
  Dataset_comparison_operators()
      : sparse_variable(makeVariable<double>({Dim::Y, Dim::Z, Dim::X},
                                             {3, 2, Dimensions::Sparse})) {
    dataset.setCoord(Dim::X, makeVariable<double>({Dim::X, 4}));
    dataset.setCoord(Dim::Y, makeVariable<double>({Dim::Y, 3}));

    dataset.setLabels("labels", makeVariable<int>({Dim::X, 4}));

    dataset.setAttr("attr", makeVariable<int>({}));

    dataset.setData("val_and_var",
                    makeVariable<double>({{Dim::Y, 3}, {Dim::X, 4}},
                                         std::vector<double>(12),
                                         std::vector<double>(12)));

    dataset.setData("val", makeVariable<double>({Dim::X, 4}));

    dataset.setSparseCoord("sparse_coord", sparse_variable);
    dataset.setData("sparse_coord_and_val", sparse_variable);
    dataset.setSparseCoord("sparse_coord_and_val", sparse_variable);
  }
  void expect_eq(const Dataset &a, const Dataset &b) const {
    expect_eq_impl(a, DatasetConstProxy(b));
    expect_eq_impl(DatasetConstProxy(a), b);
    expect_eq_impl(DatasetConstProxy(a), DatasetConstProxy(b));
  }
  void expect_ne(const Dataset &a, const Dataset &b) const {
    expect_ne_impl(a, DatasetConstProxy(b));
    expect_ne_impl(DatasetConstProxy(a), b);
    expect_ne_impl(DatasetConstProxy(a), DatasetConstProxy(b));
  }

  Dataset dataset;
  Variable sparse_variable;
};

auto make_empty() { return Dataset(); };

template <class T, class T2>
auto make_1_coord(const Dim dim, const Dimensions &dims, const units::Unit unit,
                  const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setCoord(dim, makeVariable<T>(dims, unit, data));
  return d;
}

template <class T, class T2>
auto make_1_labels(const std::string &name, const Dimensions &dims,
                   const units::Unit unit,
                   const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setLabels(name, makeVariable<T>(dims, unit, data));
  return d;
}

template <class T, class T2>
auto make_1_attr(const std::string &name, const Dimensions &dims,
                 const units::Unit unit,
                 const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setAttr(name, makeVariable<T>(dims, unit, data));
  return d;
}

template <class T, class T2>
auto make_1_values(const std::string &name, const Dimensions &dims,
                   const units::Unit unit,
                   const std::initializer_list<T2> &data) {
  auto d = make_empty();
  d.setData(name, makeVariable<T>(dims, unit, data));
  return d;
}

template <class T, class T2>
auto make_1_values_and_variances(const std::string &name,
                                 const Dimensions &dims, const units::Unit unit,
                                 const std::initializer_list<T2> &values,
                                 const std::initializer_list<T2> &variances) {
  auto d = make_empty();
  d.setData(name, makeVariable<T>(dims, unit, values, variances));
  return d;
}

// Baseline checks: Does dataset comparison pick up arbitrary mismatch of
// individual items? Strictly speaking many of these are just retesting the
// comparison of Variable, but it ensures that the content is actually compared
// and thus serves as a baseline for the follow-up tests.
TEST_F(Dataset_comparison_operators, single_coord) {
  auto d = make_1_coord<double>(Dim::X, {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_coord<float>(Dim::X, {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::Y, {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(d, make_1_coord<double>(Dim::X, {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_labels) {
  auto d = make_1_labels<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_labels<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(d, make_1_labels<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_attr) {
  auto d = make_1_attr<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_attr<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_attr<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_attr<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_attr<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(d, make_1_attr<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(d, make_1_attr<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_values) {
  auto d = make_1_values<double>("a", {Dim::X, 3}, units::m, {1, 2, 3});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_values<float>("a", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("b", {Dim::X, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::Y, 3}, units::m, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 2}, units::m, {1, 2}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 3}, units::s, {1, 2, 3}));
  expect_ne(d, make_1_values<double>("a", {Dim::X, 3}, units::m, {1, 2, 4}));
}

TEST_F(Dataset_comparison_operators, single_values_and_variances) {
  auto d = make_1_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                               {1, 2, 3}, {4, 5, 6});
  expect_eq(d, d);
  expect_ne(d, make_empty());
  expect_ne(d, make_1_values_and_variances<float>("a", {Dim::X, 3}, units::m,
                                                  {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("b", {Dim::X, 3}, units::m,
                                                   {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::Y, 3}, units::m,
                                                   {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::X, 2}, units::m,
                                                   {1, 2}, {4, 5}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::X, 3}, units::s,
                                                   {1, 2, 3}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                                   {1, 2, 4}, {4, 5, 6}));
  expect_ne(d, make_1_values_and_variances<double>("a", {Dim::X, 3}, units::m,
                                                   {1, 2, 3}, {4, 5, 7}));
}
// End baseline checks.

TEST_F(Dataset_comparison_operators, empty) {
  const auto empty = make_empty();
  expect_eq(empty, empty);
}

TEST_F(Dataset_comparison_operators, self) {
  expect_eq(dataset, dataset);
  const auto copy(dataset);
  expect_eq(copy, dataset);
}

TEST_F(Dataset_comparison_operators, extra_coord) {
  auto extra = dataset;
  extra.setCoord(Dim::Z, makeVariable<double>({Dim::Z, 2}));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_labels) {
  auto extra = dataset;
  extra.setLabels("extra", makeVariable<double>({Dim::Z, 2}));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_attr) {
  auto extra = dataset;
  extra.setAttr("extra", makeVariable<double>({Dim::Z, 2}));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_data) {
  auto extra = dataset;
  extra.setData("extra", makeVariable<double>({Dim::Z, 2}));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_variance) {
  auto extra = dataset;
  extra.setData("val", makeVariable<double>({Dim::X, 4}, std::vector<double>(4),
                                            std::vector<double>(4)));
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_sparse_values) {
  auto extra = dataset;
  extra.setData("sparse_coord", sparse_variable);
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, extra_sparse_label) {
  auto extra = dataset;
  extra.setSparseLabels("sparse_coord_and_val", "extra", sparse_variable);
  expect_ne(extra, dataset);
}

TEST_F(Dataset_comparison_operators, different_coord_insertion_order) {
  auto a = make_empty();
  auto b = make_empty();
  a.setCoord(Dim::X, dataset.coords()[Dim::X]);
  a.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  b.setCoord(Dim::X, dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, different_label_insertion_order) {
  auto a = make_empty();
  auto b = make_empty();
  a.setLabels("x", dataset.coords()[Dim::X]);
  a.setLabels("y", dataset.coords()[Dim::Y]);
  b.setLabels("y", dataset.coords()[Dim::Y]);
  b.setLabels("x", dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, different_attr_insertion_order) {
  auto a = make_empty();
  auto b = make_empty();
  a.setAttr("x", dataset.coords()[Dim::X]);
  a.setAttr("y", dataset.coords()[Dim::Y]);
  b.setAttr("y", dataset.coords()[Dim::Y]);
  b.setAttr("x", dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, different_data_insertion_order) {
  auto a = make_empty();
  auto b = make_empty();
  a.setData("x", dataset.coords()[Dim::X]);
  a.setData("y", dataset.coords()[Dim::Y]);
  b.setData("y", dataset.coords()[Dim::Y]);
  b.setData("x", dataset.coords()[Dim::X]);
  expect_eq(a, b);
}

TEST_F(Dataset_comparison_operators, with_sparse_dimension_data) {
  // a and b same, c different number of sparse values
  auto a = make_empty();
  auto data = makeVariable<double>({Dim::X, Dimensions::Sparse});
  const std::string var_name = "test_var";
  data.sparseValues<double>()[0] = {1, 2, 3};
  a.setData(var_name, data);
  auto b = make_empty();
  b.setData(var_name, data);
  expect_eq(a, b);
  data.sparseValues<double>()[0] = {2, 3, 4};
  auto c = make_empty();
  c.setData(var_name, data);
  expect_ne(a, c);
  expect_ne(b, c);
}

class Dataset3DTest : public ::testing::Test {
protected:
  Dataset3DTest() : dataset(factory.make()) {}

  Dataset datasetWithEdges(const std::initializer_list<Dim> &edgeDims) {
    auto d = dataset;
    for (const auto dim : edgeDims) {
      auto dims = dataset.coords()[dim].dims();
      dims.resize(dim, dims[dim] + 1);
      d.setCoord(dim, makeRandom(dims));
    }
    return d;
  }

  DatasetFactory3D factory;
  Dataset dataset;
};

TEST_F(Dataset3DTest, dimension_extent_check_replace_with_edge_coord) {
  auto edge_coord = dataset;
  ASSERT_NO_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 5})));
  ASSERT_NE(edge_coord["data_xyz"], dataset["data_xyz"]);
  // Cannot incrementally grow.
  ASSERT_ANY_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 6})));
  // Minor implementation shortcoming: Currently we cannot go back to non-edges.
  ASSERT_ANY_THROW(edge_coord.setCoord(Dim::X, makeRandom({Dim::X, 4})));
}

TEST_F(Dataset3DTest,
       dimension_extent_check_prevents_non_edge_coord_with_edge_data) {
  // If we reduce the X extent to 3 we would have data defined at the edges, but
  // the coord is not. This is forbidden.
  ASSERT_ANY_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 3})));
  // We *can* set data with X extent 3. The X coord is now bin edges, and other
  // data is defined on the edges.
  ASSERT_NO_THROW(dataset.setData("non_edge_data", makeRandom({Dim::X, 3})));
  // Now the X extent of the dataset is 3, but since we have data on the edges
  // we still cannot change the coord to non-edges.
  ASSERT_ANY_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 3})));
}

TEST_F(Dataset3DTest,
       dimension_extent_check_prevents_setting_edge_data_without_edge_coord) {
  ASSERT_ANY_THROW(dataset.setData("edge_data", makeRandom({Dim::X, 5})));
  ASSERT_NO_THROW(dataset.setCoord(Dim::X, makeRandom({Dim::X, 5})));
  ASSERT_NO_THROW(dataset.setData("edge_data", makeRandom({Dim::X, 5})));
}

TEST_F(Dataset3DTest, dimension_extent_check_non_coord_dimension_fail) {
  // This is the Y coordinate but has extra extent in X.
  ASSERT_ANY_THROW(
      dataset.setCoord(Dim::Y, makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
}

TEST_F(Dataset3DTest, dimension_extent_check_labels_dimension_fail) {
  // We cannot have labels on edges unless the coords are also edges. Note the
  // slight inconsistency though: Labels are typically though of as being for a
  // particular dimension (the inner one), but we can have labels on edges also
  // for the other dimensions (x in this case), just like data.
  ASSERT_ANY_THROW(
      dataset.setLabels("bad_labels", makeRandom({{Dim::X, 4}, {Dim::Y, 6}})));
  ASSERT_ANY_THROW(
      dataset.setLabels("bad_labels", makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  dataset.setCoord(Dim::Y, makeRandom({{Dim::X, 4}, {Dim::Y, 6}}));
  ASSERT_ANY_THROW(
      dataset.setLabels("bad_labels", makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  dataset.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  ASSERT_NO_THROW(
      dataset.setLabels("good_labels", makeRandom({{Dim::X, 5}, {Dim::Y, 5}})));
  ASSERT_NO_THROW(
      dataset.setLabels("good_labels", makeRandom({{Dim::X, 5}, {Dim::Y, 6}})));
  ASSERT_NO_THROW(
      dataset.setLabels("good_labels", makeRandom({{Dim::X, 4}, {Dim::Y, 6}})));
  ASSERT_NO_THROW(
      dataset.setLabels("good_labels", makeRandom({{Dim::X, 4}, {Dim::Y, 5}})));
}

class Dataset3DTest_slice_x : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {
protected:
  Dataset reference(const scipp::index pos) {
    Dataset d;
    d.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::X, pos}));
    d.setLabels("labels_xy",
                dataset.labels()["labels_xy"].slice({Dim::X, pos}));
    d.setLabels("labels_z", dataset.labels()["labels_z"]);
    d.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
    d.setData("values_x", dataset["values_x"].data().slice({Dim::X, pos}));
    d.setData("data_x", dataset["data_x"].data().slice({Dim::X, pos}));
    d.setData("data_xy", dataset["data_xy"].data().slice({Dim::X, pos}));
    d.setData("data_zyx", dataset["data_zyx"].data().slice({Dim::X, pos}));
    d.setData("data_xyz", dataset["data_xyz"].data().slice({Dim::X, pos}));
    return d;
  }
};
class Dataset3DTest_slice_y : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {};
class Dataset3DTest_slice_z : public Dataset3DTest,
                              public ::testing::WithParamInterface<int> {};

class Dataset3DTest_slice_range_x : public Dataset3DTest,
                                    public ::testing::WithParamInterface<
                                        std::pair<scipp::index, scipp::index>> {
};
class Dataset3DTest_slice_range_y : public Dataset3DTest,
                                    public ::testing::WithParamInterface<
                                        std::pair<scipp::index, scipp::index>> {
protected:
  Dataset reference(const scipp::index begin, const scipp::index end) {
    Dataset d;
    d.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
    d.setCoord(Dim::X, dataset.coords()[Dim::X]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y].slice({Dim::Y, begin, end}));
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::Y, begin, end}));
    d.setLabels("labels_x", dataset.labels()["labels_x"]);
    d.setLabels("labels_xy",
                dataset.labels()["labels_xy"].slice({Dim::Y, begin, end}));
    d.setLabels("labels_z", dataset.labels()["labels_z"]);
    d.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
    d.setAttr("attr_x", dataset.attrs()["attr_x"]);
    d.setData("data_xy", dataset["data_xy"].data().slice({Dim::Y, begin, end}));
    d.setData("data_zyx",
              dataset["data_zyx"].data().slice({Dim::Y, begin, end}));
    d.setData("data_xyz",
              dataset["data_xyz"].data().slice({Dim::Y, begin, end}));
    return d;
  }
};
class Dataset3DTest_slice_range_z : public Dataset3DTest,
                                    public ::testing::WithParamInterface<
                                        std::pair<scipp::index, scipp::index>> {
protected:
  Dataset reference(const scipp::index begin, const scipp::index end) {
    Dataset d;
    d.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
    d.setCoord(Dim::X, dataset.coords()[Dim::X]);
    d.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
    d.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::Z, begin, end}));
    d.setLabels("labels_x", dataset.labels()["labels_x"]);
    d.setLabels("labels_xy", dataset.labels()["labels_xy"]);
    d.setLabels("labels_z",
                dataset.labels()["labels_z"].slice({Dim::Z, begin, end}));
    d.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
    d.setAttr("attr_x", dataset.attrs()["attr_x"]);
    d.setData("data_zyx",
              dataset["data_zyx"].data().slice({Dim::Z, begin, end}));
    d.setData("data_xyz",
              dataset["data_xyz"].data().slice({Dim::Z, begin, end}));
    return d;
  }
};

/// Return all valid ranges (iterator pairs) for a container of given length.
template <int max> constexpr auto valid_ranges() {
  using scipp::index;
  const auto size = max + 1;
  std::array<std::pair<index, index>, (size * size + size) / 2> pairs;
  index i = 0;
  for (index first = 0; first <= max; ++first)
    for (index second = first + 0; second <= max; ++second) {
      pairs[i].first = first;
      pairs[i].second = second;
      ++i;
    }
  return pairs;
}

constexpr auto ranges_x = valid_ranges<4>();
constexpr auto ranges_y = valid_ranges<5>();
constexpr auto ranges_z = valid_ranges<6>();

INSTANTIATE_TEST_SUITE_P(AllPositions, Dataset3DTest_slice_x,
                         ::testing::Range(0, 4));
INSTANTIATE_TEST_SUITE_P(AllPositions, Dataset3DTest_slice_y,
                         ::testing::Range(0, 5));
INSTANTIATE_TEST_SUITE_P(AllPositions, Dataset3DTest_slice_z,
                         ::testing::Range(0, 6));

INSTANTIATE_TEST_SUITE_P(NonEmptyRanges, Dataset3DTest_slice_range_x,
                         ::testing::ValuesIn(ranges_x));
INSTANTIATE_TEST_SUITE_P(NonEmptyRanges, Dataset3DTest_slice_range_y,
                         ::testing::ValuesIn(ranges_y));
INSTANTIATE_TEST_SUITE_P(NonEmptyRanges, Dataset3DTest_slice_range_z,
                         ::testing::ValuesIn(ranges_z));

TEST_P(Dataset3DTest_slice_x, slice) {
  const auto pos = GetParam();
  EXPECT_EQ(dataset.slice({Dim::X, pos}), reference(pos));
}

TEST_P(Dataset3DTest_slice_x, slice_bin_edges) {
  const auto pos = GetParam();
  auto datasetWithEdges = dataset;
  datasetWithEdges.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, pos}), reference(pos));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, pos}),
            dataset.slice({Dim::X, pos}));
}

TEST_P(Dataset3DTest_slice_y, slice) {
  const auto pos = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X, dataset.coords()[Dim::X]);
  reference.setCoord(Dim::Z, dataset.coords()[Dim::Z].slice({Dim::Y, pos}));
  reference.setLabels("labels_x", dataset.labels()["labels_x"]);
  reference.setLabels("labels_z", dataset.labels()["labels_z"]);
  reference.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
  reference.setAttr("attr_x", dataset.attrs()["attr_x"]);
  reference.setData("data_xy", dataset["data_xy"].data().slice({Dim::Y, pos}));
  reference.setData("data_zyx",
                    dataset["data_zyx"].data().slice({Dim::Y, pos}));
  reference.setData("data_xyz",
                    dataset["data_xyz"].data().slice({Dim::Y, pos}));

  EXPECT_EQ(dataset.slice({Dim::Y, pos}), reference);
}

TEST_P(Dataset3DTest_slice_z, slice) {
  const auto pos = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X, dataset.coords()[Dim::X]);
  reference.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  reference.setLabels("labels_x", dataset.labels()["labels_x"]);
  reference.setLabels("labels_xy", dataset.labels()["labels_xy"]);
  reference.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
  reference.setAttr("attr_x", dataset.attrs()["attr_x"]);
  reference.setData("data_zyx",
                    dataset["data_zyx"].data().slice({Dim::Z, pos}));
  reference.setData("data_xyz",
                    dataset["data_xyz"].data().slice({Dim::Z, pos}));

  EXPECT_EQ(dataset.slice({Dim::Z, pos}), reference);
}

TEST_P(Dataset3DTest_slice_range_x, slice) {
  const auto[begin, end] = GetParam();
  Dataset reference;
  reference.setCoord(Dim::Time, dataset.coords()[Dim::Time]);
  reference.setCoord(Dim::X,
                     dataset.coords()[Dim::X].slice({Dim::X, begin, end}));
  reference.setCoord(Dim::Y, dataset.coords()[Dim::Y]);
  reference.setCoord(Dim::Z,
                     dataset.coords()[Dim::Z].slice({Dim::X, begin, end}));
  reference.setLabels("labels_x",
                      dataset.labels()["labels_x"].slice({Dim::X, begin, end}));
  reference.setLabels(
      "labels_xy", dataset.labels()["labels_xy"].slice({Dim::X, begin, end}));
  reference.setLabels("labels_z", dataset.labels()["labels_z"]);
  reference.setAttr("attr_scalar", dataset.attrs()["attr_scalar"]);
  reference.setAttr("attr_x",
                    dataset.attrs()["attr_x"].slice({Dim::X, begin, end}));
  reference.setData("values_x",
                    dataset["values_x"].data().slice({Dim::X, begin, end}));
  reference.setData("data_x",
                    dataset["data_x"].data().slice({Dim::X, begin, end}));
  reference.setData("data_xy",
                    dataset["data_xy"].data().slice({Dim::X, begin, end}));
  reference.setData("data_zyx",
                    dataset["data_zyx"].data().slice({Dim::X, begin, end}));
  reference.setData("data_xyz",
                    dataset["data_xyz"].data().slice({Dim::X, begin, end}));

  EXPECT_EQ(dataset.slice({Dim::X, begin, end}), reference);
}

TEST_P(Dataset3DTest_slice_range_y, slice) {
  const auto[begin, end] = GetParam();
  EXPECT_EQ(dataset.slice({Dim::Y, begin, end}), reference(begin, end));
}

TEST_P(Dataset3DTest_slice_range_y, slice_with_edges) {
  const auto[begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto yEdges = makeRandom({Dim::Y, 6});
  datasetWithEdges.setCoord(Dim::Y, yEdges);
  auto referenceWithEdges = reference(begin, end);
  // Is this the correct behavior for edges also in case the range is empty?
  referenceWithEdges.setCoord(Dim::Y, yEdges.slice({Dim::Y, begin, end + 1}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Y, begin, end}), referenceWithEdges);
}

TEST_P(Dataset3DTest_slice_range_y, slice_with_z_edges) {
  const auto[begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto zEdges = makeRandom({{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 7}});
  datasetWithEdges.setCoord(Dim::Z, zEdges);
  auto referenceWithEdges = reference(begin, end);
  referenceWithEdges.setCoord(Dim::Z, zEdges.slice({Dim::Y, begin, end}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Y, begin, end}), referenceWithEdges);
}

TEST_P(Dataset3DTest_slice_range_z, slice) {
  const auto[begin, end] = GetParam();
  EXPECT_EQ(dataset.slice({Dim::Z, begin, end}), reference(begin, end));
}

TEST_P(Dataset3DTest_slice_range_z, slice_with_edges) {
  const auto[begin, end] = GetParam();
  auto datasetWithEdges = dataset;
  const auto zEdges = makeRandom({{Dim::X, 4}, {Dim::Y, 5}, {Dim::Z, 7}});
  datasetWithEdges.setCoord(Dim::Z, zEdges);
  auto referenceWithEdges = reference(begin, end);
  referenceWithEdges.setCoord(Dim::Z, zEdges.slice({Dim::Z, begin, end + 1}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::Z, begin, end}), referenceWithEdges);
}

TEST_F(Dataset3DTest, nested_slice) {
  for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
    EXPECT_EQ(dataset.slice({dim, 1, 3}, {dim, 1}), dataset.slice({dim, 2}));
  }
}

TEST_F(Dataset3DTest, nested_slice_range) {
  for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
    EXPECT_EQ(dataset.slice({dim, 1, 3}, {dim, 0, 2}),
              dataset.slice({dim, 1, 3}));
    EXPECT_EQ(dataset.slice({dim, 1, 3}, {dim, 1, 2}),
              dataset.slice({dim, 2, 3}));
  }
}

TEST_F(Dataset3DTest, nested_slice_range_bin_edges) {
  auto datasetWithEdges = dataset;
  datasetWithEdges.setCoord(Dim::X, makeRandom({Dim::X, 5}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, 1, 3}, {Dim::X, 0, 2}),
            datasetWithEdges.slice({Dim::X, 1, 3}));
  EXPECT_EQ(datasetWithEdges.slice({Dim::X, 1, 3}, {Dim::X, 1, 2}),
            datasetWithEdges.slice({Dim::X, 2, 3}));
}

TEST_F(Dataset3DTest, commutative_slice) {
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2}),
            dataset.slice({Dim::Y, 2}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Y, 2}, {Dim::Z, 3, 4}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Z, 3, 4}, {Dim::Y, 2}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Z, 3, 4}, {Dim::X, 1, 3}, {Dim::Y, 2}));
}

TEST_F(Dataset3DTest, commutative_slice_range) {
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2, 4}),
            dataset.slice({Dim::Y, 2, 4}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2, 4}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Y, 2, 4}, {Dim::Z, 3, 4}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2, 4}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Z, 3, 4}, {Dim::Y, 2, 4}, {Dim::X, 1, 3}));
  EXPECT_EQ(dataset.slice({Dim::X, 1, 3}, {Dim::Y, 2, 4}, {Dim::Z, 3, 4}),
            dataset.slice({Dim::Z, 3, 4}, {Dim::X, 1, 3}, {Dim::Y, 2, 4}));
}

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

// Using typed tests for common functionality of DataProxy and DataConstProxy.
template <typename T> class DataProxyTest : public ::testing::Test {
protected:
  using dataset_type =
      std::conditional_t<std::is_same_v<T, DataProxy>, Dataset, const Dataset>;
};

using DataProxyTypes = ::testing::Types<DataProxy, DataConstProxy>;
TYPED_TEST_SUITE(DataProxyTest, DataProxyTypes);

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
  EXPECT_ANY_THROW(d_ref["sparse"].unit());
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

template <typename T> class DataProxy3DTest : public Dataset3DTest {
protected:
  using dataset_type =
      std::conditional_t<std::is_same_v<T, DataProxy>, Dataset, const Dataset>;

  dataset_type &dataset() { return Dataset3DTest::dataset; }
};

TYPED_TEST_SUITE(DataProxy3DTest, DataProxyTypes);

// We have tests that ensure that Dataset::slice is correct (and its item access
// returns the correct data), so we rely on that for verifying the results of
// slicing DataProxy.
TYPED_TEST(DataProxy3DTest, slice_single) {
  auto &d = TestFixture::dataset();
  for (const auto[name, item] : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i}), d.slice({dim, i})[name]);
        EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_length_0) {
  auto &d = TestFixture::dataset();
  for (const auto[name, item] : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 0}),
                    d.slice({dim, i, i + 0})[name]);
        EXPECT_ANY_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 0}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_length_1) {
  auto &d = TestFixture::dataset();
  for (const auto[name, item] : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, 0}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 1}),
                    d.slice({dim, i, i + 1})[name]);
        EXPECT_ANY_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 1}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice) {
  auto &d = TestFixture::dataset();
  for (const auto[name, item] : d) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1, 1}));
        for (scipp::index i = 0; i < item.dims()[dim] - 1; ++i)
          EXPECT_EQ(item.slice({dim, i, i + 2}),
                    d.slice({dim, i, i + 2})[name]);
        EXPECT_ANY_THROW(
            item.slice({dim, item.dims()[dim], item.dims()[dim] + 2}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0, 2}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_slice_range) {
  auto &d = TestFixture::dataset();
  const auto slice = d.slice({Dim::X, 2, 4});
  // Slice proxy created from DatasetProxy as opposed to directly from Dataset.
  for (const auto[name, item] : slice) {
    for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
      if (item.dims().contains(dim)) {
        EXPECT_ANY_THROW(item.slice({dim, -1}));
        for (scipp::index i = 0; i < item.dims()[dim]; ++i)
          EXPECT_EQ(item.slice({dim, i}),
                    d.slice({Dim::X, 2, 4}, {dim, i})[name]);
        EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
      } else {
        EXPECT_ANY_THROW(item.slice({dim, 0}));
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_single_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto[name, item] : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i)
            EXPECT_EQ(item.slice({dim, i}), d.slice({dim, i})[name]);
          EXPECT_ANY_THROW(item.slice({dim, item.dims()[dim]}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0}));
        }
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_length_0_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto[name, item] : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, -1}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i) {
            const auto slice = item.slice({dim, i, i + 0});
            EXPECT_EQ(slice, d.slice({dim, i, i + 0})[name]);
            if (std::set(edgeDims).count(dim)) {
              EXPECT_EQ(slice.coords()[dim].dims()[dim], 1);
            }
          }
          EXPECT_ANY_THROW(
              item.slice({dim, item.dims()[dim], item.dims()[dim] + 0}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
        }
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_length_1_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto[name, item] : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, 0}));
          for (scipp::index i = 0; i < item.dims()[dim]; ++i) {
            const auto slice = item.slice({dim, i, i + 1});
            EXPECT_EQ(slice, d.slice({dim, i, i + 1})[name]);
            if (std::set(edgeDims).count(dim)) {
              EXPECT_EQ(slice.coords()[dim].dims()[dim], 2);
            }
          }
          EXPECT_ANY_THROW(
              item.slice({dim, item.dims()[dim], item.dims()[dim] + 1}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0, 0}));
        }
      }
    }
  }
}

TYPED_TEST(DataProxy3DTest, slice_with_edges) {
  auto x = {Dim::X};
  auto xy = {Dim::X, Dim::Y};
  auto yz = {Dim::Y, Dim::Z};
  auto xyz = {Dim::X, Dim::Y, Dim::Z};
  for (const auto &edgeDims : {x, xy, yz, xyz}) {
    typename TestFixture::dataset_type d =
        TestFixture::datasetWithEdges(edgeDims);
    for (const auto[name, item] : d) {
      for (const auto dim : {Dim::X, Dim::Y, Dim::Z}) {
        if (item.dims().contains(dim)) {
          EXPECT_ANY_THROW(item.slice({dim, -1, 1}));
          for (scipp::index i = 0; i < item.dims()[dim] - 1; ++i) {
            const auto slice = item.slice({dim, i, i + 2});
            EXPECT_EQ(slice, d.slice({dim, i, i + 2})[name]);
            if (std::set(edgeDims).count(dim)) {
              EXPECT_EQ(slice.coords()[dim].dims()[dim], 3);
            }
          }
          EXPECT_ANY_THROW(
              item.slice({dim, item.dims()[dim], item.dims()[dim] + 2}));
        } else {
          EXPECT_ANY_THROW(item.slice({dim, 0, 2}));
        }
      }
    }
  }
}
