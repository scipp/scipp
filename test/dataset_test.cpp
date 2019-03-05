/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include "test_macros.h"
#include <gtest/gtest.h>

#include "convert.h"
#include "counts.h"
#include "dataset.h"
#include "dimensions.h"

TEST(Dataset, construct_empty) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, construct) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, insert_coords) {
  Dataset d;
  d.insert(Coord::Tof, {}, {1.1});
  d.insert(Coord::SpectrumNumber, {}, {2});
  EXPECT_THROW_MSG(d.insert(Coord::SpectrumNumber, {}, {2}), std::runtime_error,
                   "Attempt to insert variable with duplicate tag and name.");
  ASSERT_EQ(d.size(), 2);
}

TEST(Dataset, insert_data) {
  Dataset d;
  d.insert(Data::Value, "name1", {}, {1.1});
  d.insert(Data::Value, "name2", {}, {2});
  EXPECT_THROW_MSG(d.insert(Data::Value, "name2", {}, {2}), std::runtime_error,
                   "Attempt to insert variable with duplicate tag and name.");
  ASSERT_EQ(d.size(), 2);
}

TEST(Dataset, insert_variables_with_dimensions) {
  Dataset d;
  d.insert(Data::Value, "name1", Dimensions(Dim::Tof, 2), {1.1, 2.2});
  d.insert(Data::Value, "name2", {}, {2});
}

TEST(Dataset, insert_variables_different_order) {
  Dimensions xy;
  Dimensions xz;
  Dimensions yz;
  xy.add(Dim::X, 1);
  xz.add(Dim::X, 1);
  xy.add(Dim::Y, 2);
  yz.add(Dim::Y, 2);
  xz.add(Dim::Z, 3);
  yz.add(Dim::Z, 3);

  Dataset xyz;
  xyz.insert(Data::Value, "name1", xy, 2);
  EXPECT_NO_THROW(xyz.insert(Data::Value, "name2", yz, 6));
  EXPECT_NO_THROW(xyz.insert(Data::Value, "name3", xz, 3));

  Dataset xzy;
  xzy.insert(Data::Value, "name1", xz, 3);
  EXPECT_NO_THROW(xzy.insert(Data::Value, "name2", xy, 2));
  EXPECT_NO_THROW(xzy.insert(Data::Value, "name3", yz, 6));
}

TEST(Dataset, insert_edges) {
  Dataset d;
  d.insert(Data::Value, "name1", {Dim::Tof, 2});
  EXPECT_EQ(d.dimensions()[Dim::Tof], 2);
  EXPECT_NO_THROW(d.insert(Coord::Tof, {Dim::Tof, 3}));
  EXPECT_EQ(d.dimensions()[Dim::Tof], 2);
}

TEST(Dataset, insert_edges_first) {
  Dataset d;
  EXPECT_NO_THROW(d.insert(Coord::Tof, {Dim::Tof, 3}));
  EXPECT_EQ(d.dimensions()[Dim::Tof], 3);
  EXPECT_NO_THROW(d.insert(Data::Value, "name1", {Dim::Tof, 2}));
  EXPECT_EQ(d.dimensions()[Dim::Tof], 2);
}

TEST(Dataset, insert_edges_first_fail) {
  Dataset d;
  EXPECT_NO_THROW(d.insert(Coord::Tof, {Dim::Tof, 3}));
  EXPECT_EQ(d.dimensions()[Dim::Tof], 3);
  EXPECT_NO_THROW(d.insert(Data::Value, "name1", {Dim::Tof, 2}));
  EXPECT_EQ(d.dimensions()[Dim::Tof], 2);
  // Once we have edges and non-edges dimensions cannot change further.
  EXPECT_THROW_MSG(
      d.insert(Data::Value, "name2", {Dim::Tof, 1}), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match.");
  EXPECT_THROW_MSG(d.insert(Coord::Tof, {Dim::Tof, 4}), std::runtime_error,
                   "Attempt to insert variable with duplicate tag and name.");
}

TEST(Dataset, insert_edges_fail) {
  Dataset d;
  EXPECT_NO_THROW(d.insert(Data::Value, "name1", {Dim::Tof, 2}));
  EXPECT_EQ(d.dimensions()[Dim::Tof], 2);
  EXPECT_THROW_MSG(d.insert(Coord::Tof, {Dim::Tof, 4}), std::runtime_error,
                   "Cannot insert variable into Dataset: Variable is a "
                   "dimension coordiante, but the dimension length matches "
                   "neither as default coordinate nor as edge coordinate.");
  EXPECT_THROW_MSG(d.insert(Coord::Tof, {Dim::Tof, 1}), std::runtime_error,
                   "Cannot insert variable into Dataset: Variable is a "
                   "dimension coordiante, but the dimension length matches "
                   "neither as default coordinate nor as edge coordinate.");
}

TEST(Dataset, insert_edges_reverse_fail) {
  Dataset d;
  EXPECT_NO_THROW(d.insert(Coord::Tof, {Dim::Tof, 3}));
  EXPECT_EQ(d.dimensions()[Dim::Tof], 3);
  EXPECT_THROW_MSG(
      d.insert(Data::Value, "name1", Dimensions(Dim::Tof, 1)),
      std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match.");
  EXPECT_THROW_MSG(
      d.insert(Data::Value, "name1", Dimensions(Dim::Tof, 4)),
      std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match.");
}

TEST(Dataset, can_use_normal_insert_to_copy_edges) {
  Dataset d;
  d.insert(Data::Value, "", {Dim::X, 2});
  d.insert(Coord::X, {Dim::X, 3});

  Dataset copy;
  for (auto var : d)
    EXPECT_NO_THROW(copy.insert(var));
}

TEST(Dataset, custom_type) {
  Dataset d;
  d.insert<float>(Data::Value, "", {Dim::Tof, 2});
  EXPECT_EQ(d(Data::Value, "").dtype(), dtype<float>);
  EXPECT_TRUE(
      (std::is_same<decltype(d(Data::Value, "").span<float>())::element_type,
                    float>::value));
}

TEST(Dataset, mixed_type_operations_fails_currently) {
  // This *currently* fails, but we would eventually want to support this.
  Dataset d1;
  d1.insert<float>(Data::Value, "", {});
  Dataset d2;
  d2.insert<double>(Data::Value, "", {});
  EXPECT_NO_THROW(d1 += d1);
  EXPECT_NO_THROW(d2 += d2);
  EXPECT_ANY_THROW(d1 += d2);
}

TEST(Dataset, get_variable_view) {
  Dataset d;
  d.insert(Data::Value, "", {});
  d.insert(Data::Value, "name", {});
  d.insert(Coord::X, {});

  EXPECT_EQ(d(Coord::X).tag(), Coord::X);
  EXPECT_EQ(d(Data::Value, "").tag(), Data::Value);
  EXPECT_EQ(d(Data::Value, "").name(), "");
  EXPECT_EQ(d(Data::Value, "name").tag(), Data::Value);
  EXPECT_EQ(d(Data::Value, "name").name(), "name");
  EXPECT_THROW_MSG_SUBSTR(d(Coord::Y), dataset::except::VariableNotFoundError,
                          "could not find variable with tag "
                          "Coord::Y and name ``");
}

TEST(Dataset, extract) {
  Dataset d;
  d.insert(Data::Value, "name1", {}, {1.1});
  d.insert(Data::Variance, "name1", {}, {1.1});
  d.insert(Data::Value, "name2", {}, {2});
  EXPECT_EQ(d.size(), 3);
  auto name1 = d.extract("name1");
  EXPECT_EQ(d.size(), 1);
  EXPECT_EQ(name1.size(), 2);
  auto name2 = d.extract("name2");
  EXPECT_EQ(d.size(), 0);
  EXPECT_EQ(name2.size(), 1);
}

TEST(Dataset, merge) {
  Dataset d;
  d.insert(Data::Value, "name1", {}, {1.1});
  d.insert(Data::Variance, "name1", {}, {1.1});
  d.insert(Data::Value, "name2", {}, {2});

  Dataset merged;
  merged.merge(d);
  EXPECT_EQ(merged.size(), 3);
  EXPECT_THROW_MSG(merged.merge(d), std::runtime_error,
                   "Attempt to insert variable with duplicate tag and name.");

  Dataset d2;
  d2.insert(Data::Value, "name3", {}, {1.1});
  merged.merge(d2);
  EXPECT_EQ(merged.size(), 4);
}

TEST(Dataset, merge_matching_coordinates) {
  Dataset d1;
  d1.insert(Coord::X, {Dim::X, 2}, {1.1, 2.2});
  d1.insert(Data::Value, "data1", {Dim::X, 2});

  Dataset d2;
  d2.insert(Coord::X, {Dim::X, 2}, {1.1, 2.2});
  d2.insert(Data::Value, "data2", {Dim::X, 2});

  ASSERT_NO_THROW(d1.merge(d2));
  EXPECT_EQ(d1.size(), 3);
}

TEST(Dataset, merge_coord_mismatch_fail) {
  Dataset d1;
  d1.insert(Coord::X, {Dim::X, 2}, {1.1, 2.2});
  d1.insert(Data::Value, "data1", {Dim::X, 2});

  Dataset d2;
  d2.insert(Coord::X, {Dim::X, 2}, {1.1, 2.3});
  d2.insert(Data::Value, "data2", {Dim::X, 2});

  EXPECT_THROW_MSG(d1.merge(d2), std::runtime_error,
                   "Cannot merge: Coordinates do not match.");
}

TEST(Dataset, const_get) {
  Dataset d;
  d.insert(Data::Value, "", {}, {1.1});
  d.insert(Data::Variance, "", {}, {2});
  const auto &const_d(d);
  auto view = const_d.get(Data::Value);
  // No non-const access to variable if Dataset is const, will not compile:
  // auto &view = const_d.get(Data::Value);
  ASSERT_EQ(view.size(), 1);
  EXPECT_EQ(view[0], 1.1);
  // auto is deduced to be const, so assignment will not compile:
  // view[0] = 1.2;
}

TEST(Dataset, get) {
  Dataset d;
  d.insert(Data::Value, "", {}, {1.1});
  d.insert(Data::Variance, "", {}, {2});
  auto view = d.get(Data::Value);
  ASSERT_EQ(view.size(), 1);
  EXPECT_EQ(view[0], 1.1);
  view[0] = 2.2;
  EXPECT_EQ(view[0], 2.2);
}

TEST(Dataset, get_const) {
  Dataset d;
  d.insert(Data::Value, "", {}, {1.1});
  d.insert(Data::Variance, "", {}, {2});
  auto view = d.get(Data::Value);
  ASSERT_EQ(view.size(), 1);
  EXPECT_EQ(view[0], 1.1);
  // auto is now deduced to be const, so assignment will not compile:
  // view[0] = 1.2;
}

TEST(Dataset, get_fail) {
  Dataset d;
  d.insert(Data::Value, "name1", {}, {1.1});
  d.insert(Data::Value, "name2", {}, {1.1});
  EXPECT_THROW_MSG_SUBSTR(d.get(Data::Value), std::runtime_error,
                          "could not find variable with tag "
                          "Data::Value and name ``.");
  EXPECT_THROW_MSG_SUBSTR(d.get(Data::Variance),
                          dataset::except::VariableNotFoundError,
                          "could not find variable with tag "
                          "Data::Variance and name ``.");
}

TEST(Dataset, get_named) {
  Dataset d;
  d.insert(Data::Value, "name1", {}, {1.1});
  d.insert(Data::Value, "name2", {}, {2.2});
  auto var1 = d.get(Data::Value, "name1");
  ASSERT_EQ(var1.size(), 1);
  EXPECT_EQ(var1[0], 1.1);
  auto var2 = d.get(Data::Value, "name2");
  ASSERT_EQ(var2.size(), 1);
  EXPECT_EQ(var2[0], 2.2);
}

TEST(Dataset, comparison_different_insertion_order) {
  Dataset d1;
  d1.insert(Data::Value, "a", {});
  d1.insert(Data::Value, "b", {});
  Dataset d2;
  d2.insert(Data::Value, "b", {});
  d2.insert(Data::Value, "a", {});
  EXPECT_EQ(d1, d1);
  EXPECT_EQ(d1, d2);
  EXPECT_EQ(d2, d1);
  EXPECT_EQ(d2, d2);
}

TEST(Dataset, comparison_different_data) {
  Dataset d1;
  d1.insert(Data::Value, "a", {});
  d1.insert(Data::Value, "b", {});
  Dataset d2;
  d2.insert(Data::Value, "b", {});
  d2.insert(Data::Value, "a", {}, {1.0});
  EXPECT_EQ(d1, d1);
  EXPECT_NE(d1, d2);
  EXPECT_NE(d2, d1);
  EXPECT_EQ(d2, d2);
}

TEST(Dataset, comparison_missing_variable) {
  Dataset d1;
  d1.insert(Data::Value, "a", {});
  d1.insert(Data::Value, "b", {});
  Dataset d2;
  d2.insert(Data::Value, "a", {});
  EXPECT_EQ(d1, d1);
  EXPECT_NE(d1, d2);
  EXPECT_NE(d2, d1);
  EXPECT_EQ(d2, d2);
}

TEST(Dataset, comparison_with_subset) {
  Dataset d1;
  d1.insert(Data::Value, "a", {});
  d1.insert(Data::Variance, "a", {});
  Dataset d2;
  d2.insert(Data::Value, "b", {});
  d2.insert(Data::Value, "a", {});
  d2.insert(Data::Variance, "a", {});
  EXPECT_NE(d1, d2);
  EXPECT_EQ(d1, d2.subset("a"));
  EXPECT_EQ(d2.subset("a"), d1);
}

TEST(Dataset, subset) {
  Dataset d;
  d.insert(Coord::X, {});
  d.insert(Data::Value, "a", {});
  d.insert(Data::Variance, "a", {});
  d.insert(Data::Value, "b", {});
  d.insert(Data::Variance, "b", {});

  const auto no_data = d.subset("");
  EXPECT_EQ(no_data.size(), 1);
  EXPECT_TRUE(no_data.contains(Coord::X));

  const auto value = d.subset(Data::Value, "a");
  EXPECT_EQ(value.size(), 2);
  EXPECT_TRUE(value.contains(Coord::X));
  EXPECT_TRUE(value.contains(Data::Value, "a"));

  const auto variance = d.subset(Data::Variance, "a");
  EXPECT_EQ(variance.size(), 2);
  EXPECT_TRUE(variance.contains(Coord::X));
  EXPECT_TRUE(variance.contains(Data::Variance, "a"));

  const auto both = d.subset("a");
  EXPECT_EQ(both.size(), 3);
  EXPECT_TRUE(both.contains(Coord::X));
  EXPECT_TRUE(both.contains(Data::Value, "a"));
  EXPECT_TRUE(both.contains(Data::Variance, "a"));
}

TEST(Dataset, subset_of_subset) {
  Dataset d;
  d.insert(Coord::X, {});
  d.insert(Data::Value, "a", {});
  d.insert(Data::Variance, "a", {});
  d.insert(Data::Value, "b", {});
  d.insert(Data::Variance, "b", {});

  const auto value = d.subset(Data::Value, "a");
  const auto both = d.subset("a");

  const auto value_from_subset = both.subset(Data::Value, "a");

  EXPECT_EQ(value, value_from_subset);
  EXPECT_EQ(value_from_subset.size(), 2);
  EXPECT_TRUE(value_from_subset.contains(Coord::X));
  EXPECT_TRUE(value_from_subset.contains(Data::Value, "a"));
}

TEST(Dataset, subset_of_full_subset) {
  Dataset d;
  d.insert(Coord::X, {});
  d.insert(Data::Value, "a", {});
  d.insert(Data::Variance, "a", {});
  d.insert(Data::Value, "b", {});
  d.insert(Data::Variance, "b", {});

  const auto both = d.subset("a");
  const DatasetSlice full(d);
  EXPECT_EQ(full.size(), 5);

  const auto both_from_subset = full.subset("a");

  EXPECT_EQ(both, both_from_subset);
  EXPECT_EQ(both_from_subset.size(), 3);
  EXPECT_TRUE(both_from_subset.contains(Coord::X));
  EXPECT_TRUE(both_from_subset.contains(Data::Value, "a"));
  EXPECT_TRUE(both_from_subset.contains(Data::Variance, "a"));
}

TEST(Dataset, comparison_with_spatial_slice) {
  Dataset d1;
  d1.insert(Data::Value, "a", {Dim::X, 2}, {2, 3});
  Dataset d2;
  d2.insert(Data::Value, "b", {});
  d2.insert(Data::Value, "a", {Dim::X, 3}, {1, 2, 3});

  EXPECT_NE(d1, d2);

  EXPECT_NE(d1, d2.subset("a"));
  EXPECT_NE(d1, d2.subset("a")(Dim::X, 0, 2));
  EXPECT_NE(d1, d2.subset("a")(Dim::X, 0));
  EXPECT_NE(d1, d2.subset("a")(Dim::X, 1));
  EXPECT_EQ(d1, d2.subset("a")(Dim::X, 1, 3));

  EXPECT_NE(d2.subset("a"), d1);
  EXPECT_NE(d2.subset("a")(Dim::X, 0, 2), d1);
  EXPECT_NE(d2.subset("a")(Dim::X, 0), d1);
  EXPECT_NE(d2.subset("a")(Dim::X, 1), d1);
  EXPECT_EQ(d2.subset("a")(Dim::X, 1, 3), d1);
}

TEST(Dataset, comparison_two_slices) {
  Dataset d;
  d.insert(Data::Value, "a", {Dim::X, 4}, {1, 2, 3, 4});
  d.insert(Data::Value, "b", {Dim::X, 4}, {1, 2, 1, 2});

  // Data is same but name differs.
  EXPECT_NE(d.subset("a")(Dim::X, 0, 2), d.subset("b")(Dim::X, 0, 2));

  EXPECT_EQ(d.subset("a")(Dim::X, 0, 2), d.subset("a")(Dim::X, 0, 2));
  EXPECT_NE(d.subset("a")(Dim::X, 0, 2), d.subset("a")(Dim::X, 1, 3));
  EXPECT_NE(d.subset("a")(Dim::X, 0, 2), d.subset("a")(Dim::X, 2, 4));

  EXPECT_EQ(d.subset("b")(Dim::X, 0, 2), d.subset("b")(Dim::X, 0, 2));
  EXPECT_NE(d.subset("b")(Dim::X, 0, 2), d.subset("b")(Dim::X, 1, 3));
  EXPECT_EQ(d.subset("b")(Dim::X, 0, 2), d.subset("b")(Dim::X, 2, 4));
}

TEST(Dataset, operator_plus_equal) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", {Dim::X, 1}, {2.2});
  a += a;
  EXPECT_EQ(a.get(Coord::X)[0], 0.1);
  EXPECT_EQ(a.get(Data::Value)[0], 4.4);
}

TEST(Dataset, operator_plus_equal_broadcast) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 1}}),
           {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  Dataset b;
  b.insert(Coord::X, {Dim::X, 1}, {0.1});
  b.insert(Data::Value, "", Dimensions({{Dim::Z, 3}}), {0.1, 0.2, 0.3});

  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.get(Coord::X)[0], 0.1);
  EXPECT_EQ(a.get(Data::Value)[0], 1.1);
  EXPECT_EQ(a.get(Data::Value)[1], 2.1);
  EXPECT_EQ(a.get(Data::Value)[2], 3.2);
  EXPECT_EQ(a.get(Data::Value)[3], 4.2);
  EXPECT_EQ(a.get(Data::Value)[4], 5.3);
  EXPECT_EQ(a.get(Data::Value)[5], 6.3);
}

TEST(Dataset, operator_plus_equal_transpose) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 1}}),
           {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  Dataset b;
  b.insert(Coord::X, {Dim::X, 1}, {0.1});
  b.insert(Data::Value, "", Dimensions({{Dim::Y, 2}, {Dim::Z, 3}}),
           {0.1, 0.2, 0.3, 0.1, 0.2, 0.3});

  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.get(Coord::X)[0], 0.1);
  EXPECT_EQ(a.get(Data::Value)[0], 1.1);
  EXPECT_EQ(a.get(Data::Value)[1], 2.1);
  EXPECT_EQ(a.get(Data::Value)[2], 3.2);
  EXPECT_EQ(a.get(Data::Value)[3], 4.2);
  EXPECT_EQ(a.get(Data::Value)[4], 5.3);
  EXPECT_EQ(a.get(Data::Value)[5], 6.3);
}

TEST(Dataset, operator_plus_equal_different_content) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "name1", {Dim::X, 1}, {2.2});
  Dataset b;
  b.insert(Coord::X, {Dim::X, 1}, {0.1});
  b.insert(Data::Value, "name1", {Dim::X, 1}, {2.2});
  b.insert(Data::Value, "name2", {Dim::X, 1}, {3.3});
  EXPECT_THROW_MSG(a += b, std::runtime_error,
                   "Right-hand-side in binary operation contains variable that "
                   "is not present in left-hand-side.");
  EXPECT_NO_THROW(b += a);
}

TEST(Dataset, operator_plus_equal_with_attributes) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", {Dim::X, 1}, {2.2});
  Dataset logs;
  logs.insert<std::string>(Data::Value, "comments", {}, {std::string("test")});
  a.insert(Attr::ExperimentLog, "", {}, {logs});
  a += a;
  EXPECT_EQ(a.get(Coord::X)[0], 0.1);
  EXPECT_EQ(a.get(Data::Value)[0], 4.4);
  // For now there is no special merging behavior, just keep attributes of first
  // operand.
  EXPECT_EQ(a.get(Attr::ExperimentLog)[0], logs);
}

TEST(Dataset, operator_times_equal) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", {Dim::X, 1}, {3.0});
  a *= a;
  EXPECT_EQ(a.get(Coord::X)[0], 0.1);
  EXPECT_EQ(a.get(Data::Value)[0], 9.0);
}

TEST(Dataset, operator_times_equal_with_attributes) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", {Dim::X, 1}, {3.0});
  Dataset logs;
  logs.insert<std::string>(Data::Value, "comments", {}, {std::string("test")});
  a.insert(Attr::ExperimentLog, "", {}, {logs});
  a *= a;
  EXPECT_EQ(a.get(Coord::X)[0], 0.1);
  EXPECT_EQ(a.get(Data::Value)[0], 9.0);
  EXPECT_EQ(a.get(Attr::ExperimentLog)[0], logs);
}

TEST(Dataset, operator_times_equal_with_uncertainty) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", {Dim::X, 1}, {3.0});
  a.insert(Data::Variance, "", {Dim::X, 1}, {2.0});
  Dataset b;
  b.insert(Coord::X, {Dim::X, 1}, {0.1});
  b.insert(Data::Value, "", {Dim::X, 1}, {4.0});
  b.insert(Data::Variance, "", {Dim::X, 1}, {3.0});
  a *= b;
  EXPECT_EQ(a.get(Coord::X)[0], 0.1);
  EXPECT_EQ(a.get(Data::Value)[0], 12.0);
  EXPECT_EQ(a.get(Data::Variance)[0], 2.0 * 16.0 + 3.0 * 9.0);
}

TEST(Dataset, operator_times_equal_uncertainty_failures) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "name1", {Dim::X, 1}, {3.0});
  a.insert(Data::Variance, "name1", {Dim::X, 1}, {2.0});
  Dataset b;
  b.insert(Coord::X, {Dim::X, 1}, {0.1});
  b.insert(Data::Value, "name1", {Dim::X, 1}, {4.0});
  Dataset c;
  c.insert(Coord::X, {Dim::X, 1}, {0.1});
  c.insert(Data::Variance, "name1", {Dim::X, 1}, {2.0});
  EXPECT_THROW_MSG(a *= b, std::runtime_error,
                   "Either both or none of the operands must have a variance "
                   "for their values.");
  EXPECT_THROW_MSG(b *= a, std::runtime_error,
                   "Either both or none of the operands must have a variance "
                   "for their values.");
  EXPECT_THROW_MSG(c *= c, std::runtime_error,
                   "Cannot multiply datasets that contain a variance but no "
                   "corresponding value.");
  EXPECT_THROW_MSG(a *= c, std::runtime_error,
                   "Cannot multiply datasets that contain a variance but no "
                   "corresponding value.");
  EXPECT_THROW_MSG(c *= a, std::runtime_error,
                   "Right-hand-side in addition contains variable that is not "
                   "present in left-hand-side.");
  EXPECT_THROW_MSG(b *= c, std::runtime_error,
                   "Right-hand-side in addition contains variable that is not "
                   "present in left-hand-side.");
  EXPECT_THROW_MSG(c *= b, std::runtime_error,
                   "Right-hand-side in addition contains variable that is not "
                   "present in left-hand-side.");
}

TEST(Dataset, operator_times_equal_with_units) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  Variable values(Data::Value, Dimensions({{Dim::X, 1}}), {3.0});
  values.setUnit(units::m);
  Variable variances(Data::Variance, Dimensions({{Dim::X, 1}}), {2.0});
  variances.setUnit(units::m * units::m);
  a.insert(values);
  a.insert(variances);
  a *= a;
  EXPECT_EQ(a(Data::Value).unit(), units::m * units::m);
  EXPECT_EQ(a(Data::Variance).unit(),
            units::m * units::m * units::m * units::m);
  EXPECT_EQ(a.get(Data::Variance)[0], 36.0);
}

TEST(Dataset, operator_times_equal_histogram_data) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  Variable values(Data::Value, Dimensions({{Dim::X, 1}}), {3.0});
  values.setName("name1");
  values.setUnit(units::counts);
  Variable variances(Data::Variance, Dimensions({{Dim::X, 1}}), {2.0});
  variances.setName("name1");
  variances.setUnit(units::counts * units::counts);
  a.insert(values);
  a.insert(variances);

  Dataset b;
  b.insert(Coord::X, {Dim::X, 1}, {0.1});
  b.insert(Data::Value, "name1", {Dim::X, 1}, {4.0});
  b.insert(Data::Variance, "name1", {Dim::X, 1}, {4.0});

  // Counts (aka "histogram data") times counts not possible.
  EXPECT_THROW_MSG(
      a *= a, std::runtime_error,
      "Unsupported unit as result of multiplication: (counts^2) * (counts^2)");
  // Counts times frequencies (aka "distribution") ok.
  // TODO Works for dimensionless right now, but do we need to handle other
  // cases as well?
  auto a_copy(a);
  EXPECT_NO_THROW(a *= b);
  EXPECT_NO_THROW(b *= a_copy);
}

TEST(Dataset, operator_plus_with_temporary_avoids_copy) {
  Dataset a;
  a.insert(Data::Value, "", {Dim::X, 1}, {2.2});
  const auto a2(a);
  const auto b(a);

  const auto *addr = &a.get(Data::Value)[0];
  auto sum = std::move(a) + b;
  EXPECT_EQ(&sum.get(Data::Value)[0], addr);

  const auto *addr2 = &a2.get(Data::Value)[0];
  auto sum2 = a2 + b;
  EXPECT_NE(&sum2.get(Data::Value)[0], addr2);
}

TEST(Dataset, slice) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 2}, {0.0, 0.1});
  d.insert(Data::Value, "", {{Dim::Y, 3}, {Dim::X, 2}},
           {0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
  for (const gsl::index i : {0, 1}) {
    Dataset sliceX = d(Dim::X, i);
    ASSERT_EQ(sliceX.size(), 1);
    ASSERT_EQ(sliceX.get(Data::Value).size(), 3);
    EXPECT_EQ(sliceX.get(Data::Value)[0], 0.0 + i);
    EXPECT_EQ(sliceX.get(Data::Value)[1], 2.0 + i);
    EXPECT_EQ(sliceX.get(Data::Value)[2], 4.0 + i);
  }
  for (const gsl::index i : {0, 1}) {
    Dataset sliceX = d(Dim::X, i, i + 1);
    ASSERT_EQ(sliceX.size(), 2);
    ASSERT_EQ(sliceX.get(Coord::X).size(), 1);
    EXPECT_EQ(sliceX.get(Coord::X)[0], 0.1 * i);
    ASSERT_EQ(sliceX.get(Data::Value).size(), 3);
    EXPECT_EQ(sliceX.get(Data::Value)[0], 0.0 + i);
    EXPECT_EQ(sliceX.get(Data::Value)[1], 2.0 + i);
    EXPECT_EQ(sliceX.get(Data::Value)[2], 4.0 + i);
  }
  for (const gsl::index i : {0, 1, 2}) {
    Dataset sliceY = d(Dim::Y, i);
    ASSERT_EQ(sliceY.size(), 2);
    ASSERT_EQ(sliceY.get(Coord::X), d.get(Coord::X));
    ASSERT_EQ(sliceY.get(Data::Value).size(), 2);
    EXPECT_EQ(sliceY.get(Data::Value)[0], 0.0 + 2 * i);
    EXPECT_EQ(sliceY.get(Data::Value)[1], 1.0 + 2 * i);
  }
  EXPECT_THROW_MSG(
      d(Dim::Z, 0), std::runtime_error,
      "Expected dimension to be in {{Dim::Y, 3}, {Dim::X, 2}}, got Dim::Z.");
  EXPECT_THROW_MSG(
      d(Dim::Z, 1), std::runtime_error,
      "Expected dimension to be in {{Dim::Y, 3}, {Dim::X, 2}}, got Dim::Z.");
}

TEST(Dataset, concatenate_constant_dimension_broken) {
  Dataset a;
  a.insert(Data::Value, "name1", {}, {1.1});
  a.insert(Data::Value, "name2", {}, {2.2});
  auto d = concatenate(a, a, Dim::X);
  // TODO Special case: No variable depends on X so the result does not contain
  // this dimension either. Change this behavior?!
  EXPECT_FALSE(d.dimensions().contains(Dim::X));
}

TEST(Dataset, concatenate) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", {Dim::X, 1}, {2.2});
  auto x = concatenate(a, a, Dim::X);
  EXPECT_TRUE(x.dimensions().contains(Dim::X));
  EXPECT_EQ(x.get(Coord::X).size(), 2);
  EXPECT_EQ(x.get(Data::Value).size(), 2);
  auto x2(x);
  x2.get(Data::Value)[0] = 100.0;
  auto xy = concatenate(x, x2, Dim::Y);
  EXPECT_TRUE(xy.dimensions().contains(Dim::X));
  EXPECT_TRUE(xy.dimensions().contains(Dim::Y));
  EXPECT_EQ(xy.get(Coord::X).size(), 2);
  EXPECT_EQ(xy.get(Data::Value).size(), 4);

  xy = concatenate(xy, x, Dim::Y);
  EXPECT_EQ(xy.get(Coord::X).size(), 2);
  EXPECT_EQ(xy.get(Data::Value).size(), 6);

  xy = concatenate(xy, xy, Dim::Y);
  EXPECT_EQ(xy.get(Coord::X).size(), 2);
  EXPECT_EQ(xy.get(Data::Value).size(), 12);
}

TEST(Dataset, concatenate_with_bin_edges) {
  Dataset ds;
  ds.insert(Coord::X, {Dim::X, 2}, {0.1, 0.2});
  ds.insert(Data::Value, "", {Dim::X, 1}, {2.2});
  EXPECT_NO_THROW(concatenate(ds, ds, Dim::Y));

  Dataset not_edge;
  not_edge.insert(Coord::X, {Dim::X, 1}, {0.3});
  not_edge.insert(Data::Value, "", {Dim::X, 1}, {2.2});
  EXPECT_THROW_MSG(
      concatenate(ds, not_edge, Dim::X), std::runtime_error,
      "Cannot concatenate: Second variable is not an edge variable.");
  not_edge.erase(Coord::X);
  not_edge.insert(Coord::X, {}, {0.3});
  EXPECT_THROW_MSG(concatenate(ds, not_edge, Dim::X),
                   dataset::except::DimensionNotFoundError,
                   "Expected dimension to be in {}, got Dim::X.");

  EXPECT_THROW_MSG(concatenate(ds, ds, Dim::X), std::runtime_error,
                   "Cannot concatenate: Last bin edge of first edge variable "
                   "does not match first bin edge of second edge variable.");

  Dataset ds2;
  ds2.insert(Coord::X, {Dim::X, 2}, {0.2, 0.3});
  ds2.insert(Data::Value, "", {Dim::X, 1}, {3.3});

  Dataset merged;
  EXPECT_NO_THROW(merged = concatenate(ds, ds2, Dim::X));
  EXPECT_EQ(merged.dimensions().count(), 1);
  EXPECT_TRUE(merged.dimensions().contains(Dim::X));
  EXPECT_TRUE(equals(merged.get(Coord::X), {0.1, 0.2, 0.3}));
  EXPECT_TRUE(equals(merged.get(Data::Value), {2.2, 3.3}));
}

TEST(Dataset, concatenate_with_varying_bin_edges) {
  Dataset ds;
  ds.insert(Coord::X, {{Dim::Y, 2}, {Dim::X, 2}}, {0.1, 0.2, 0.11, 0.21});
  ds.insert(Data::Value, "", {{Dim::Y, 2}, {Dim::X, 1}}, {2.2, 3.3});

  Dataset ds2;
  ds2.insert(Coord::X, {{Dim::Y, 2}, {Dim::X, 2}}, {0.2, 0.3, 0.21, 0.31});
  ds2.insert(Data::Value, "", {{Dim::Y, 2}, {Dim::X, 1}}, {4.4, 5.5});

  Dataset merged;
  merged = concatenate(ds, ds2, Dim::X);
  ASSERT_NO_THROW(merged = concatenate(ds, ds2, Dim::X));
  ASSERT_EQ(merged.dimensions().count(), 2);
  ASSERT_TRUE(merged.dimensions().contains(Dim::X));
  ASSERT_TRUE(merged.dimensions().contains(Dim::Y));
  EXPECT_EQ(merged.dimensions()[Dim::X], 2);
  EXPECT_EQ(merged.dimensions()[Dim::Y], 2);
  EXPECT_TRUE(equals(merged.get(Coord::X), {0.1, 0.2, 0.3, 0.11, 0.21, 0.31}));
  EXPECT_TRUE(equals(merged.get(Data::Value), {2.2, 4.4, 3.3, 5.5}));
}

TEST(Dataset, concatenate_with_attributes) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 1}, {0.1});
  a.insert(Data::Value, "", {Dim::X, 1}, {2.2});
  Dataset logs;
  logs.insert<std::string>(Data::Value, "comments", {}, {std::string("test")});
  a.insert(Attr::ExperimentLog, "", {}, {logs});

  auto x = concatenate(a, a, Dim::X);
  EXPECT_TRUE(x.dimensions().contains(Dim::X));
  EXPECT_EQ(x.get(Coord::X).size(), 2);
  EXPECT_EQ(x.get(Data::Value).size(), 2);
  EXPECT_EQ(x.get(Attr::ExperimentLog).size(), 1);
  EXPECT_EQ(x.get(Attr::ExperimentLog)[0], logs);

  auto x2(x);
  x2.get(Data::Value)[0] = 100.0;
  x2.get(Attr::ExperimentLog)[0].span<std::string>(Data::Value, "comments")[0] =
      "different";
  auto xy = concatenate(x, x2, Dim::Y);
  EXPECT_TRUE(xy.dimensions().contains(Dim::X));
  EXPECT_TRUE(xy.dimensions().contains(Dim::Y));
  EXPECT_EQ(xy.get(Coord::X).size(), 2);
  EXPECT_EQ(xy.get(Data::Value).size(), 4);
  // Attributes get a dimension, no merging happens. This might be useful
  // behavior, e.g., when dealing with multiple runs in a single dataset?
  EXPECT_EQ(xy.get(Attr::ExperimentLog).size(), 2);
  EXPECT_EQ(xy.get(Attr::ExperimentLog)[0], logs);

  EXPECT_NO_THROW(concatenate(xy, xy, Dim::X));

  auto xy2(xy);
  xy2.get(Attr::ExperimentLog)[0].span<std::string>(Data::Value,
                                                    "comments")[0] = "";
  // Concatenating in existing dimension fail currently. Would need to implement
  // merging functionality for attributes?
  EXPECT_ANY_THROW(concatenate(xy, xy2, Dim::X));
}

TEST(Dataset, rebin_failures) {
  Dataset d;
  Variable coord(Coord::X, {Dim::X, 3}, {1.0, 3.0, 5.0});
  EXPECT_THROW_MSG_SUBSTR(rebin(d, coord),
                          dataset::except::VariableNotFoundError,
                          "could not find variable with tag "
                          "Coord::X and name ``");
  Variable data(Data::Value, {Dim::X, 2}, {2.0, 4.0});
  EXPECT_THROW_MSG(
      rebin(d, data), std::runtime_error,
      "The provided rebin coordinate is not a coordinate variable.");
  Variable nonDimCoord(Coord::Mask, {Dim::Detector, 2});
  EXPECT_THROW_MSG(
      rebin(d, nonDimCoord), std::runtime_error,
      "The provided rebin coordinate is not a dimension coordinate.");
  Variable missingDimCoord(Coord::X, {Dim::Y, 2}, {2.0, 4.0});
  EXPECT_THROW_MSG(rebin(d, missingDimCoord), std::runtime_error,
                   "The provided rebin coordinate lacks the dimension "
                   "corresponding to the coordinate.");
  Variable nonContinuousCoord(Coord::SpectrumNumber, {Dim::Spectrum, 2},
                              {2.0, 4.0});
  EXPECT_THROW_MSG(
      rebin(d, nonContinuousCoord), std::runtime_error,
      "The provided rebin coordinate is not a continuous coordinate.");
  Variable oldMissingDimCoord(Coord::X, {Dim::Y, 3}, {1.0, 3.0, 5.0});
  d.insert(oldMissingDimCoord);
  EXPECT_THROW_MSG(rebin(d, coord), std::runtime_error,
                   "Existing coordinate to be rebined lacks the dimension "
                   "corresponding to the new coordinate.");
  d.erase(Coord::X);
  d.insert(coord);
  EXPECT_THROW_MSG(rebin(d, coord), std::runtime_error,
                   "Existing coordinate to be rebinned is not a bin edge "
                   "coordinate. Use `resample` instead of rebin or convert to "
                   "histogram data first.");
  d.erase(Coord::X);
  d.insert(coord);
  d.insert(Data::Value, "badAuxDim", Dimensions({{Dim::X, 2}, {Dim::Y, 2}}));
  d(Data::Value, "badAuxDim").setUnit(units::counts);
  Variable badAuxDim(Coord::X, Dimensions({{Dim::X, 3}, {Dim::Y, 3}}));
  EXPECT_THROW_MSG(rebin(d, badAuxDim), std::runtime_error,
                   "Size mismatch in auxiliary dimension of new coordinate.");
}

TEST(Dataset, rebin_accepts_only_counts_and_densities) {
  Dataset d;
  d.insert(Coord::Tof, {Dim::Tof, 3}, {1.0, 3.0, 5.0});
  Variable coordNew(Coord::Tof, {Dim::Tof, 2}, {1.0, 5.0});

  d.insert(Data::Value, "", {Dim::Tof, 2}, {10.0, 20.0});
  EXPECT_THROW_MSG(rebin(d, coordNew), dataset::except::UnitError,
                   "Expected counts or counts-density, got dimensionless.");

  d(Data::Value, "").setUnit(units::m);
  EXPECT_THROW_MSG(rebin(d, coordNew), dataset::except::UnitError,
                   "Expected counts or counts-density, got m.");

  d(Data::Value, "").setUnit(units::counts);
  EXPECT_NO_THROW(rebin(d, coordNew));

  d(Data::Value, "").setUnit(units::counts * units::counts);
  EXPECT_NO_THROW(rebin(d, coordNew));

  d(Data::Value, "").setUnit(units::counts / units::us);
  EXPECT_NO_THROW(rebin(d, coordNew));
  rebin(d, coordNew);
}

TEST(Dataset, rebin) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 3}, {1.0, 3.0, 5.0});
  Variable coordNew(Coord::X, {Dim::X, 2}, {1.0, 5.0});
  // With only the coord in the dataset there is no way to tell it is an edge,
  // so this fails.
  EXPECT_THROW_MSG(rebin(d, coordNew), std::runtime_error,
                   "Existing coordinate to be rebinned is not a bin edge "
                   "coordinate. Use `resample` instead of rebin or convert to "
                   "histogram data first.");

  d.insert(Data::Value, "", {Dim::X, 2}, {10.0, 20.0});
  d(Data::Value).setUnit(units::counts);
  auto rebinned = rebin(d, coordNew);
  EXPECT_EQ(rebinned.get(Data::Value).size(), 1);
  EXPECT_EQ(rebinned.get(Data::Value)[0], 30.0);
}

TEST(Dataset, rebin_density) {
  Dataset d;
  d.insert(Coord::Tof, {Dim::Tof, 4}, {1, 2, 4, 8});
  Variable coordNew(Coord::Tof, {Dim::Tof, 3}, {1, 3, 8});

  d.insert(Data::Value, "", {Dim::Tof, 3}, {10, 20, 30});
  d(Data::Value).setUnit(units::counts);

  Variable reference(Data::Value, {Dim::Tof, 2}, {10.0, 40.0 / 5});
  reference.setUnit(units::counts / units::us);

  auto rebinned1 = rebin(counts::toDensity(d, Dim::Tof), coordNew);
  auto rebinned2 = counts::toDensity(rebin(d, coordNew), Dim::Tof);
  EXPECT_EQ(rebinned1, rebinned2);
  EXPECT_EQ(rebinned1(Data::Value), reference);
}

Dataset makeEvents() {
  Dataset e1;
  e1.insert(Data::Tof, "", {Dim::Event, 5}, {1, 2, 3, 4, 5});
  Dataset e2;
  e2.insert(Data::Tof, "", {Dim::Event, 7}, {1, 2, 3, 4, 4, 5, 7});
  Dataset d;
  d.insert(Data::Events, "sample1", {Dim::Spectrum, 2}, {e1, e2});
  return d;
}

TEST(Dataset, histogram_failures) {
  auto d = makeEvents();

  Dataset dependsOnBinDim;
  dependsOnBinDim.insert(d(Data::Events, "sample1").reshape({Dim::Tof, 2}));
  Variable coord(Coord::Tof, {Dim::Tof, 3}, {1.0, 1.5, 4.5});
  EXPECT_THROW_MSG(histogram(dependsOnBinDim, coord), std::runtime_error,
                   "Data to histogram depends on histogram dimension.");

  Variable coordWithExtraDim(Coord::Tof, {{Dim::X, 2}, {Dim::Tof, 3}},
                             {1.0, 1.5, 4.5, 1.5, 4.5, 7.5});
  EXPECT_THROW(histogram(d, coordWithExtraDim),
               dataset::except::DimensionNotFoundError);

  Variable coordWithLengthMismatch(Coord::Tof,
                                   {{Dim::Spectrum, 3}, {Dim::Tof, 3}});
  EXPECT_THROW(histogram(d, coordWithLengthMismatch),
               dataset::except::DimensionLengthError);

  Variable coordNotIncreasing(Coord::Tof, {Dim::Tof, 3}, {1.0, 1.5, 1.4});
  EXPECT_THROW_MSG(histogram(d, coordNotIncreasing), std::runtime_error,
                   "Coordinate used for binning is not increasing.");
}

TEST(Dataset, histogram) {
  auto d = makeEvents();
  Variable coord(Coord::Tof, {Dim::Tof, 3}, {1.0, 1.5, 4.5});
  auto hist = histogram(d, coord);

  ASSERT_TRUE(hist.contains(Coord::Tof));
  EXPECT_EQ(hist(Coord::Tof), coord);
  ASSERT_TRUE(hist.contains(Data::Value, "sample1"));
  ASSERT_TRUE(hist.contains(Data::Variance, "sample1"));
  EXPECT_TRUE(equals(hist.get(Data::Value, "sample1"), {1, 3, 1, 4}));
  EXPECT_TRUE(equals(hist.get(Data::Variance, "sample1"), {1, 3, 1, 4}));
  EXPECT_EQ(hist(Data::Value, "sample1").unit(), units::counts);
  EXPECT_EQ(hist(Data::Variance, "sample1").unit(),
            units::counts * units::counts);
}

TEST(Dataset, histogram_2D_coord) {
  auto d = makeEvents();
  Variable coord(Coord::Tof, {{Dim::Spectrum, 2}, {Dim::Tof, 3}},
                 {1.0, 1.5, 4.5, 1.5, 4.5, 7.5});
  auto hist = histogram(d, coord);

  ASSERT_TRUE(hist.contains(Coord::Tof));
  EXPECT_EQ(hist(Coord::Tof), coord);
  ASSERT_TRUE(hist.contains(Data::Value, "sample1"));
  ASSERT_TRUE(hist.contains(Data::Variance, "sample1"));
  EXPECT_TRUE(equals(hist.get(Data::Value, "sample1"), {1, 3, 4, 2}));
  EXPECT_TRUE(equals(hist.get(Data::Variance, "sample1"), {1, 3, 4, 2}));
  EXPECT_EQ(hist(Data::Value, "sample1").unit(), units::counts);
  EXPECT_EQ(hist(Data::Variance, "sample1").unit(),
            units::counts * units::counts);
}

TEST(Dataset, histogram_2D_transpose_coord) {
  auto d = makeEvents();
  Variable coord(Coord::Tof, {{Dim::Tof, 3}, {Dim::Spectrum, 2}},
                 {1.0, 1.5, 1.5, 4.5, 4.5, 7.5});
  auto hist = histogram(d, coord);

  ASSERT_TRUE(hist.contains(Coord::Tof));
  EXPECT_EQ(hist(Coord::Tof), coord);
  ASSERT_TRUE(hist.contains(Data::Value, "sample1"));
  ASSERT_TRUE(hist.contains(Data::Variance, "sample1"));
  // Dimensionality of output is determined by that of the input events, the bin
  // dimension will almost be the innermost one.
  ASSERT_EQ(hist(Data::Value, "sample1").dimensions(),
            Dimensions({{Dim::Spectrum, 2}, {Dim::Tof, 2}}));
  EXPECT_TRUE(equals(hist.get(Data::Value, "sample1"), {1, 3, 4, 2}));
  EXPECT_TRUE(equals(hist.get(Data::Variance, "sample1"), {1, 3, 4, 2}));
  EXPECT_EQ(hist(Data::Value, "sample1").unit(), units::counts);
  EXPECT_EQ(hist(Data::Variance, "sample1").unit(),
            units::counts * units::counts);
}

TEST(Dataset, sort) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 4}, {5.0, 1.0, 3.0, 0.0});
  d.insert(Coord::Y, {Dim::Y, 2}, {1.0, 0.9});
  d.insert(Data::Value, "", {Dim::X, 4}, {1.0, 2.0, 3.0, 4.0});

  auto sorted = sort(d, Coord::X);

  ASSERT_EQ(sorted.get(Coord::X).size(), 4);
  EXPECT_EQ(sorted.get(Coord::X)[0], 0.0);
  EXPECT_EQ(sorted.get(Coord::X)[1], 1.0);
  EXPECT_EQ(sorted.get(Coord::X)[2], 3.0);
  EXPECT_EQ(sorted.get(Coord::X)[3], 5.0);

  ASSERT_EQ(sorted.get(Coord::Y).size(), 2);
  EXPECT_EQ(sorted.get(Coord::Y)[0], 1.0);
  EXPECT_EQ(sorted.get(Coord::Y)[1], 0.9);

  ASSERT_EQ(sorted.get(Data::Value).size(), 4);
  EXPECT_EQ(sorted.get(Data::Value)[0], 4.0);
  EXPECT_EQ(sorted.get(Data::Value)[1], 2.0);
  EXPECT_EQ(sorted.get(Data::Value)[2], 3.0);
  EXPECT_EQ(sorted.get(Data::Value)[3], 1.0);
}

TEST(Dataset, sort_2D) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 4}, {5.0, 1.0, 3.0, 0.0});
  d.insert(Coord::Y, {Dim::Y, 2}, {1.0, 0.9});
  d.insert(Data::Value, "", {{Dim::Y, 2}, {Dim::X, 4}},
           {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});

  auto sorted = sort(d, Coord::X);

  ASSERT_EQ(sorted.get(Coord::X).size(), 4);
  EXPECT_EQ(sorted.get(Coord::X)[0], 0.0);
  EXPECT_EQ(sorted.get(Coord::X)[1], 1.0);
  EXPECT_EQ(sorted.get(Coord::X)[2], 3.0);
  EXPECT_EQ(sorted.get(Coord::X)[3], 5.0);

  ASSERT_EQ(sorted.get(Coord::Y).size(), 2);
  EXPECT_EQ(sorted.get(Coord::Y)[0], 1.0);
  EXPECT_EQ(sorted.get(Coord::Y)[1], 0.9);

  ASSERT_EQ(sorted.get(Data::Value).size(), 8);
  EXPECT_EQ(sorted.get(Data::Value)[0], 4.0);
  EXPECT_EQ(sorted.get(Data::Value)[1], 2.0);
  EXPECT_EQ(sorted.get(Data::Value)[2], 3.0);
  EXPECT_EQ(sorted.get(Data::Value)[3], 1.0);
  EXPECT_EQ(sorted.get(Data::Value)[4], 8.0);
  EXPECT_EQ(sorted.get(Data::Value)[5], 6.0);
  EXPECT_EQ(sorted.get(Data::Value)[6], 7.0);
  EXPECT_EQ(sorted.get(Data::Value)[7], 5.0);
}

TEST(Dataset, filter) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 4}, {5.0, 1.0, 3.0, 0.0});
  d.insert(Coord::Y, {Dim::Y, 2}, {1.0, 0.9});
  d.insert(Data::Value, "", {{Dim::Y, 2}, {Dim::X, 4}},
           {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0});
  Variable select(Coord::Mask, {Dim::X, 4}, {false, true, false, true});

  auto filtered = filter(d, select);

  ASSERT_EQ(filtered.get(Coord::X).size(), 2);
  EXPECT_EQ(filtered.get(Coord::X)[0], 1.0);
  EXPECT_EQ(filtered.get(Coord::X)[1], 0.0);

  ASSERT_EQ(filtered.get(Coord::Y).size(), 2);
  EXPECT_EQ(filtered.get(Coord::Y)[0], 1.0);
  EXPECT_EQ(filtered.get(Coord::Y)[1], 0.9);

  ASSERT_EQ(filtered.get(Data::Value).size(), 4);
  EXPECT_EQ(filtered.get(Data::Value)[0], 2.0);
  EXPECT_EQ(filtered.get(Data::Value)[1], 4.0);
  EXPECT_EQ(filtered.get(Data::Value)[2], 6.0);
  EXPECT_EQ(filtered.get(Data::Value)[3], 8.0);
}

TEST(Dataset, integrate_counts) {
  Dataset ds;
  ds.insert(Coord::X, {Dim::X, 3}, {0.1, 0.2, 0.4});
  ds.insert(Data::Value, "", {Dim::X, 2}, {10.0, 20.0});
  ds(Data::Value, "").setUnit(units::counts);

  // Note that in this special case the integral has the same unit. This is
  // maybe an indicator that we should rather use `sum` for counts? On the other
  // hand, supporting `integrate` is convenient and thanks to the unit this
  // should be safe.
  Variable reference(Data::Value, {}, {30.0});
  reference.setUnit(units::counts);

  Dataset integral;
  integral = integrate(ds, Dim::X);
  EXPECT_NO_THROW(integral = integrate(ds, Dim::X));
  EXPECT_EQ(integral.dimensions().count(), 0);
  EXPECT_FALSE(integral.contains(Coord::X));
  EXPECT_EQ(integral(Data::Value), reference);
}

TEST(Dataset, integrate_counts_density) {
  Dataset ds;
  ds.insert(Coord::Tof, {Dim::Tof, 3}, {0.1, 0.2, 0.4});
  ds.insert(Data::Value, "", {Dim::Tof, 2}, {10.0, 20.0});
  ds(Data::Value, "").setUnit(units::counts / units::us);

  Variable reference(Data::Value, {}, {10.0 * 0.1 + 20.0 * 0.2});
  reference.setUnit(units::counts);

  Dataset integral;
  EXPECT_NO_THROW(integral = integrate(ds, Dim::Tof));
  EXPECT_EQ(integral.dimensions().count(), 0);
  EXPECT_FALSE(integral.contains(Coord::Tof));
  EXPECT_EQ(integral(Data::Value), reference);
}

TEST(DatasetSlice, basics) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 4});
  d.insert(Coord::Y, {Dim::Y, 2});
  d.insert(Data::Value, "a", {{Dim::Y, 2}, {Dim::X, 4}});
  d.insert(Data::Value, "b", {{Dim::Y, 2}, {Dim::X, 4}});
  d.insert(Data::Variance, "a", {{Dim::Y, 2}, {Dim::X, 4}});
  d.insert(Data::Variance, "b", {{Dim::Y, 2}, {Dim::X, 4}});

  ConstDatasetSlice viewA(d, "a");
  ConstDatasetSlice viewB(d, "b");

  auto check = [](const auto &view, const std::string &name) {
    ASSERT_EQ(view.size(), 4);
    gsl::index count = 0;
    for (const auto &var : view) {
      if (var.isData()) {
        EXPECT_EQ(var.name(), name);
        ++count;
      }
    }
    EXPECT_EQ(count, 2);
  };

  check(viewA, "a");
  check(viewB, "b");
  check(d.subset("a"), "a");
  check(d.subset("b"), "b");
}

TEST(DatasetSlice, minus_equals) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 4});
  d.insert(Coord::Y, {Dim::Y, 2});
  d.insert(Data::Value, "a", {{Dim::Y, 2}, {Dim::X, 4}}, 8, 1.0);
  d.insert(Data::Value, "b", {{Dim::Y, 2}, {Dim::X, 4}}, 8, 1.0);
  d.insert(Data::Variance, "a", {{Dim::Y, 2}, {Dim::X, 4}}, 8, 1.0);
  d.insert(Data::Variance, "b", {{Dim::Y, 2}, {Dim::X, 4}}, 8, 1.0);

  EXPECT_NO_THROW(d -= d.subset("a"));

  EXPECT_EQ(d.get(Data::Value, "a")[0], 0.0);
  EXPECT_EQ(d.get(Data::Value, "b")[0], 1.0);
  EXPECT_EQ(d.get(Data::Variance, "a")[0], 2.0);
  EXPECT_EQ(d.get(Data::Variance, "b")[0], 1.0);

  ASSERT_NO_THROW(d.subset("a") -= d.subset("b"));

  ASSERT_EQ(d.size(), 6);
  // Note: Variable not renamed when operating with slices.
  EXPECT_EQ(d.get(Data::Value, "a")[0], -1.0);
  EXPECT_EQ(d.get(Data::Value, "b")[0], 1.0);
  EXPECT_EQ(d.get(Data::Variance, "a")[0], 3.0);
  EXPECT_EQ(d.get(Data::Variance, "b")[0], 1.0);
}

TEST(DatasetSlice, slice_spatial) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 4}, {1, 2, 3, 4});
  d.insert(Coord::Y, {Dim::Y, 2}, {1, 2});
  d.insert(Data::Value, "a", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});
  d.insert(Data::Variance, "a", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});

  auto view_x13 = d(Dim::X, 1, 3);
  ASSERT_EQ(view_x13.size(), 4);
  EXPECT_EQ(view_x13[0].dimensions(), (Dimensions{Dim::X, 2}));
  EXPECT_EQ(view_x13[1].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_x13[2].dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_EQ(view_x13[3].dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
}

TEST(DatasetSlice, subset_slice_spatial) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 4}, {1, 2, 3, 4});
  d.insert(Coord::Y, {Dim::Y, 2}, {1, 2});
  d.insert(Data::Value, "a", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});
  d.insert(Data::Value, "b", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});
  d.insert(Data::Variance, "a", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});
  d.insert(Data::Variance, "b", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});

  auto view_a_x0 = d.subset("a")(Dim::X, 0);

  // Slice with single index (not range) => corresponding dimension coordinate
  // is removed.
  ASSERT_EQ(view_a_x0.size(), 3);
  EXPECT_EQ(view_a_x0[0].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_a_x0[1].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_a_x0[2].dimensions(), (Dimensions{Dim::Y, 2}));

  auto view_a_x1 = d.subset("a")(Dim::X, 1);

  ASSERT_EQ(view_a_x1.size(), 3);
  EXPECT_EQ(view_a_x1[0].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_a_x1[1].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_a_x1[2].dimensions(), (Dimensions{Dim::Y, 2}));

  EXPECT_NO_THROW(view_a_x1 -= view_a_x0);

  EXPECT_TRUE(equals(d.get(Coord::X), {1, 2, 3, 4}));
  EXPECT_TRUE(equals(d.get(Coord::Y), {1, 2}));
  EXPECT_TRUE(equals(d.get(Data::Value, "a"), {1, 1, 3, 4, 5, 1, 7, 8}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "a"), {1, 3, 3, 4, 5, 11, 7, 8}));
  EXPECT_TRUE(equals(d.get(Data::Value, "b"), {1, 2, 3, 4, 5, 6, 7, 8}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "b"), {1, 2, 3, 4, 5, 6, 7, 8}));

  // If we slice with a range index the corresponding coordinate (and dimension)
  // is preserved, even if the range has size 1. Thus the operation fails due to
  // coordinate mismatch, as it should.
  auto view_a_x01 = d.subset("a")(Dim::X, 0, 1);
  auto view_a_x12 = d.subset("a")(Dim::X, 1, 2);
  EXPECT_THROW_MSG_SUBSTR(view_a_x12 -= view_a_x01,
                          dataset::except::VariableMismatchError,
                          "expected to match");
}

TEST(DatasetSlice, subset_slice_spatial_with_bin_edges) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 5}, {1, 2, 3, 4, 5});
  d.insert(Coord::Y, {Dim::Y, 2}, {1, 2});
  d.insert(Data::Value, "a", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});
  d.insert(Data::Value, "b", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});
  d.insert(Data::Variance, "a", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});
  d.insert(Data::Variance, "b", {{Dim::Y, 2}, {Dim::X, 4}},
           {1, 2, 3, 4, 5, 6, 7, 8});

  auto view_a_x0 = d.subset("a")(Dim::X, 0);

  // Slice with single index (not range) => corresponding dimension coordinate
  // is removed.
  ASSERT_EQ(view_a_x0.size(), 3);
  EXPECT_EQ(view_a_x0[0].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_a_x0[1].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_a_x0[2].dimensions(), (Dimensions{Dim::Y, 2}));

  auto view_a_x1 = d.subset("a")(Dim::X, 1);

  ASSERT_EQ(view_a_x1.size(), 3);
  EXPECT_EQ(view_a_x1[0].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_a_x1[1].dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_EQ(view_a_x1[2].dimensions(), (Dimensions{Dim::Y, 2}));

  EXPECT_NO_THROW(view_a_x1 -= view_a_x0);

  EXPECT_TRUE(equals(d.get(Coord::X), {1, 2, 3, 4, 5}));
  EXPECT_TRUE(equals(d.get(Coord::Y), {1, 2}));
  EXPECT_TRUE(equals(d.get(Data::Value, "a"), {1, 1, 3, 4, 5, 1, 7, 8}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "a"), {1, 3, 3, 4, 5, 11, 7, 8}));
  EXPECT_TRUE(equals(d.get(Data::Value, "b"), {1, 2, 3, 4, 5, 6, 7, 8}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "b"), {1, 2, 3, 4, 5, 6, 7, 8}));

  auto view_a_x01 = d.subset("a")(Dim::X, 0, 1);
  auto view_a_x12 = d.subset("a")(Dim::X, 1, 2);
  ASSERT_EQ(view_a_x01[0].tag(), Coord::X);
  // View extent is 1 so we get 2 edges.
  ASSERT_EQ(view_a_x01.dimensions()[Dim::X], 1);
  ASSERT_EQ(view_a_x01[0].dimensions()[Dim::X], 2);
  EXPECT_TRUE(equals(view_a_x01[0].get(Coord::X), {1, 2}));
  EXPECT_TRUE(equals(view_a_x12[0].get(Coord::X), {2, 3}));

  auto view_a_x02 = d.subset("a")(Dim::X, 0, 2);
  auto view_a_x13 = d.subset("a")(Dim::X, 1, 3);
  ASSERT_EQ(view_a_x02[0].tag(), Coord::X);
  // View extent is 2 so we get 3 edges.
  ASSERT_EQ(view_a_x02.dimensions()[Dim::X], 2);
  ASSERT_EQ(view_a_x02[0].dimensions()[Dim::X], 3);
  EXPECT_TRUE(equals(view_a_x02[0].get(Coord::X), {1, 2, 3}));
  EXPECT_TRUE(equals(view_a_x13[0].get(Coord::X), {2, 3, 4}));

  // If we slice with a range index the corresponding coordinate (and dimension)
  // is preserved, even if the range has size 1. Thus the operation fails due to
  // coordinate mismatch, as it should.
  EXPECT_THROW_MSG_SUBSTR(view_a_x12 -= view_a_x01,
                          dataset::except::VariableMismatchError,
                          "expected to match");
  EXPECT_THROW_MSG_SUBSTR(view_a_x13 -= view_a_x02,
                          dataset::except::VariableMismatchError,
                          "expected to match");
}

TEST(Dataset, unary_minus) {
  Dataset a;
  a.insert(Coord::X, {Dim::X, 2}, {1, 2});
  a.insert(Data::Value, "a", {Dim::X, 2}, {1, 2});
  a.insert(Data::Value, "b", {}, {3});
  a.insert(Data::Variance, "a", {Dim::X, 2}, {4, 5});
  a.insert(Data::Variance, "b", {}, {6});

  auto b = -a;
  EXPECT_EQ(b(Coord::X), a(Coord::X));
  EXPECT_EQ(b(Data::Value, "a"), -a(Data::Value, "a"));
  EXPECT_EQ(b(Data::Value, "b"), -a(Data::Value, "b"));
  // Note variance not changing sign.
  EXPECT_EQ(b(Data::Variance, "a"), a(Data::Variance, "a"));
  EXPECT_EQ(b(Data::Variance, "b"), a(Data::Variance, "b"));
}

TEST(Dataset, binary_assign_with_scalar) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 2}, {1, 2});
  d.insert(Data::Value, "d1", {Dim::X, 2}, {1, 2});
  d.insert(Data::Value, "d2", {}, {3});
  d.insert(Data::Variance, "d1", {Dim::X, 2}, {4, 5});
  d.insert(Data::Variance, "d2", {}, {6});

  d += 1;
  EXPECT_TRUE(equals(d.get(Data::Value, "d1"), {2, 3}));
  EXPECT_TRUE(equals(d.get(Data::Value, "d2"), {4}));
  // Scalar treated as having 0 variance, `+` leaves variance unchanged.
  EXPECT_TRUE(equals(d.get(Data::Variance, "d1"), {4, 5}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "d2"), {6}));

  d -= 2;
  EXPECT_TRUE(equals(d.get(Data::Value, "d1"), {0, 1}));
  EXPECT_TRUE(equals(d.get(Data::Value, "d2"), {2}));
  // Scalar treated as having 0 variance, `-` leaves variance unchanged.
  EXPECT_TRUE(equals(d.get(Data::Variance, "d1"), {4, 5}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "d2"), {6}));

  d *= 2;
  EXPECT_TRUE(equals(d.get(Data::Value, "d1"), {0, 2}));
  EXPECT_TRUE(equals(d.get(Data::Value, "d2"), {4}));
  // Scalar treated as having 0 variance, `*` affects variance.
  EXPECT_TRUE(equals(d.get(Data::Variance, "d1"), {16, 20}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "d2"), {24}));
}

TEST(DatasetSlice, binary_assign_with_scalar) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 2}, {1, 2});
  d.insert(Data::Value, "a", {Dim::X, 2}, {1, 2});
  d.insert(Data::Value, "b", {}, {3});
  d.insert(Data::Variance, "a", {Dim::X, 2}, {4, 5});
  d.insert(Data::Variance, "b", {}, {6});

  auto slice = d(Dim::X, 1);

  slice += 1;
  EXPECT_TRUE(equals(d.get(Data::Value, "a"), {1, 3}));
  // TODO This behavior should be reconsidered and probably change: A slice
  // should not include variables that do not have the dimension, otherwise,
  // e.g., looping over slices will apply an operation to that variable more
  // than once.
  EXPECT_TRUE(equals(d.get(Data::Value, "b"), {4}));
  // Scalar treated as having 0 variance, `+` leaves variance unchanged.
  EXPECT_TRUE(equals(d.get(Data::Variance, "a"), {4, 5}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "b"), {6}));

  slice -= 2;
  EXPECT_TRUE(equals(d.get(Data::Value, "a"), {1, 1}));
  EXPECT_TRUE(equals(d.get(Data::Value, "b"), {2}));
  // Scalar treated as having 0 variance, `-` leaves variance unchanged.
  EXPECT_TRUE(equals(d.get(Data::Variance, "a"), {4, 5}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "b"), {6}));

  slice *= 2;
  EXPECT_TRUE(equals(d.get(Data::Value, "a"), {1, 2}));
  EXPECT_TRUE(equals(d.get(Data::Value, "b"), {4}));
  // Scalar treated as having 0 variance, `*` affects variance.
  EXPECT_TRUE(equals(d.get(Data::Variance, "a"), {4, 20}));
  EXPECT_TRUE(equals(d.get(Data::Variance, "b"), {24}));
}

TEST(Dataset, binary_with_scalar) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 2}, {1, 2});
  d.insert(Data::Value, "a", {Dim::X, 2}, {1, 2});
  d.insert(Data::Value, "b", {}, {3});
  d.insert(Data::Variance, "a", {Dim::X, 2}, {4, 5});
  d.insert(Data::Variance, "b", {}, {6});

  auto sum = d + 1;
  EXPECT_TRUE(equals(sum.get(Data::Value, "a"), {2, 3}));
  EXPECT_TRUE(equals(sum.get(Data::Value, "b"), {4}));
  EXPECT_TRUE(equals(sum.get(Data::Variance, "a"), {4, 5}));
  EXPECT_TRUE(equals(sum.get(Data::Variance, "b"), {6}));
  sum = 2 + d;
  EXPECT_TRUE(equals(sum.get(Data::Value, "a"), {3, 4}));
  EXPECT_TRUE(equals(sum.get(Data::Value, "b"), {5}));
  EXPECT_TRUE(equals(sum.get(Data::Variance, "a"), {4, 5}));
  EXPECT_TRUE(equals(sum.get(Data::Variance, "b"), {6}));

  auto diff = d - 1;
  EXPECT_TRUE(equals(diff.get(Data::Value, "a"), {0, 1}));
  EXPECT_TRUE(equals(diff.get(Data::Value, "b"), {2}));
  EXPECT_TRUE(equals(diff.get(Data::Variance, "a"), {4, 5}));
  EXPECT_TRUE(equals(diff.get(Data::Variance, "b"), {6}));
  diff = 2 - d;
  EXPECT_TRUE(equals(diff.get(Data::Value, "a"), {1, 0}));
  EXPECT_TRUE(equals(diff.get(Data::Value, "b"), {-1}));
  EXPECT_TRUE(equals(diff.get(Data::Variance, "a"), {4, 5}));
  EXPECT_TRUE(equals(diff.get(Data::Variance, "b"), {6}));

  auto prod = d * 2;
  EXPECT_TRUE(equals(prod.get(Data::Value, "a"), {2, 4}));
  EXPECT_TRUE(equals(prod.get(Data::Value, "b"), {6}));
  EXPECT_TRUE(equals(prod.get(Data::Variance, "a"), {16, 20}));
  EXPECT_TRUE(equals(prod.get(Data::Variance, "b"), {24}));
  prod = 3 * d;
  EXPECT_TRUE(equals(prod.get(Data::Value, "a"), {3, 6}));
  EXPECT_TRUE(equals(prod.get(Data::Value, "b"), {9}));
  EXPECT_TRUE(equals(prod.get(Data::Variance, "a"), {36, 45}));
  EXPECT_TRUE(equals(prod.get(Data::Variance, "b"), {54}));
}

TEST(DatasetSlice, binary_with_scalar) {
  Dataset d;
  d.insert(Coord::X, {Dim::X, 2}, {1, 2});
  d.insert(Data::Value, "a", {Dim::X, 2}, {1, 2});
  d.insert(Data::Value, "b", {}, {3});
  d.insert(Data::Variance, "a", {Dim::X, 2}, {4, 5});
  d.insert(Data::Variance, "b", {}, {6});

  auto slice = d(Dim::X, 1);

  // Note that these operations actually work by implicitly converting
  // DatasetSlice to Dataset, so this test is actually testing that conversion,
  // not the binary operation iteself.
  auto sum = slice + 1;
  EXPECT_TRUE(equals(sum.get(Data::Value, "a"), {3}));
  EXPECT_TRUE(equals(sum.get(Data::Value, "b"), {4}));
  EXPECT_TRUE(equals(sum.get(Data::Variance, "a"), {5}));
  EXPECT_TRUE(equals(sum.get(Data::Variance, "b"), {6}));
  sum = 2 + slice;
  EXPECT_TRUE(equals(sum.get(Data::Value, "a"), {4}));
  EXPECT_TRUE(equals(sum.get(Data::Value, "b"), {5}));
  EXPECT_TRUE(equals(sum.get(Data::Variance, "a"), {5}));
  EXPECT_TRUE(equals(sum.get(Data::Variance, "b"), {6}));

  auto diff = slice - 1;
  EXPECT_TRUE(equals(diff.get(Data::Value, "a"), {1}));
  EXPECT_TRUE(equals(diff.get(Data::Value, "b"), {2}));
  EXPECT_TRUE(equals(diff.get(Data::Variance, "a"), {5}));
  EXPECT_TRUE(equals(diff.get(Data::Variance, "b"), {6}));
  diff = 2 - slice;
  EXPECT_TRUE(equals(diff.get(Data::Value, "a"), {0}));
  EXPECT_TRUE(equals(diff.get(Data::Value, "b"), {-1}));
  EXPECT_TRUE(equals(diff.get(Data::Variance, "a"), {5}));
  EXPECT_TRUE(equals(diff.get(Data::Variance, "b"), {6}));

  auto prod = slice * 2;
  EXPECT_TRUE(equals(prod.get(Data::Value, "a"), {4}));
  EXPECT_TRUE(equals(prod.get(Data::Value, "b"), {6}));
  EXPECT_TRUE(equals(prod.get(Data::Variance, "a"), {20}));
  EXPECT_TRUE(equals(prod.get(Data::Variance, "b"), {24}));
  prod = 3 * slice;
  EXPECT_TRUE(equals(prod.get(Data::Value, "a"), {6}));
  EXPECT_TRUE(equals(prod.get(Data::Value, "b"), {9}));
  EXPECT_TRUE(equals(prod.get(Data::Variance, "a"), {45}));
  EXPECT_TRUE(equals(prod.get(Data::Variance, "b"), {54}));
}

TEST(Dataset, counts_toDensity_fromDensity) {
  Dataset d;
  d.insert(Coord::Tof, {Dim::Tof, 4}, {1, 2, 4, 8});
  d.insert(Data::Value, "", {Dim::Tof, 3}, {12, 12, 12});
  d(Data::Value, "").setUnit(units::counts);

  d = counts::toDensity(std::move(d), Dim::Tof);
  auto result = d(Data::Value, "");
  EXPECT_EQ(result.unit(), units::counts / units::us);
  EXPECT_TRUE(equals(result.get(Data::Value), {12, 6, 3}));

  d = counts::fromDensity(std::move(d), Dim::Tof);
  result = d(Data::Value, "");
  EXPECT_EQ(result.unit(), units::counts);
  EXPECT_TRUE(equals(result.get(Data::Value), {12, 12, 12}));
}
