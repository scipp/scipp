/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>

#include "test_macros.h"

#include "dataset.h"
#include "dimensions.h"

TEST(Dataset, construct_empty) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, construct) { ASSERT_NO_THROW(Dataset d); }

TEST(Dataset, insert_coords) {
  Dataset d;
  d.insert<Coord::Tof>(Dimensions{}, {1.1});
  d.insert<Coord::SpectrumNumber>(Dimensions{}, {2});
  EXPECT_THROW_MSG(d.insert<Coord::SpectrumNumber>(Dimensions{}, {2}),
                   std::runtime_error,
                   "Attempt to insert duplicate coordinate.");
  ASSERT_EQ(d.size(), 2);
}

TEST(Dataset, insert_data) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
  EXPECT_THROW_MSG(d.insert<Data::Int>("name2", Dimensions{}, {2l}),
                   std::runtime_error,
                   "Attempt to insert data of same type with duplicate name.");
  ASSERT_EQ(d.size(), 2);
}

TEST(Dataset, insert_variables_with_dimensions) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
}

TEST(Dataset, insert_variables_different_order) {
  Dimensions xy;
  Dimensions xz;
  Dimensions yz;
  xy.add(Dimension::X, 1);
  xz.add(Dimension::X, 1);
  xy.add(Dimension::Y, 2);
  yz.add(Dimension::Y, 2);
  xz.add(Dimension::Z, 3);
  yz.add(Dimension::Z, 3);

  Dataset xyz;
  xyz.insert<Data::Value>("name1", xy, 2);
  EXPECT_NO_THROW(xyz.insert<Data::Value>("name2", yz, 6));
  EXPECT_NO_THROW(xyz.insert<Data::Value>("name3", xz, 3));

  Dataset xzy;
  xzy.insert<Data::Value>("name1", xz, 3);
  EXPECT_NO_THROW(xzy.insert<Data::Value>("name2", xy, 2));
  EXPECT_NO_THROW(xzy.insert<Data::Value>("name3", yz, 6));
}

TEST(Dataset, insertAsEdge) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  auto edges = makeVariable<Data::Variance>(Dimensions(Dimension::Tof, 3),
                                            {1.1, 2.2, 3.3});
  edges.setName("edges");
  EXPECT_EQ(d.dimensions().size(Dimension::Tof), 2);
  EXPECT_THROW_MSG(
      d.insert(edges), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
  EXPECT_NO_THROW(d.insertAsEdge(Dimension::Tof, edges));
}

TEST(Dataset, insertAsEdge_fail) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  auto too_short =
      makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  EXPECT_THROW_MSG(
      d.insertAsEdge(Dimension::Tof, too_short), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
  auto too_long = makeVariable<Data::Value>(Dimensions(Dimension::Tof, 4),
                                            {1.1, 2.2, 3.3, 4.4});
  EXPECT_THROW_MSG(
      d.insertAsEdge(Dimension::Tof, too_long), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
  auto edges =
      makeVariable<Data::Value>(Dimensions(Dimension::Tof, 3), {1.1, 2.2, 3.3});
  EXPECT_THROW_MSG(d.insertAsEdge(Dimension::X, edges), std::runtime_error,
                   "Dimension not found.");
}

TEST(Dataset, insertAsEdge_reverse_fail) {
  Dataset d;
  auto edges =
      makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  d.insertAsEdge(Dimension::Tof, edges);
  EXPECT_THROW_MSG(
      d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2}),
      std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
}

TEST(Dataset, extract) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Variance>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
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
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Variance>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});

  Dataset merged;
  merged.merge(d);
  EXPECT_EQ(merged.size(), 3);
  EXPECT_THROW_MSG(merged.merge(d), std::runtime_error,
                   "Attempt to insert data of same type with duplicate name.");

  Dataset d2;
  d2.insert<Data::Value>("name3", Dimensions{}, {1.1});
  merged.merge(d2);
  EXPECT_EQ(merged.size(), 4);
}

TEST(Dataset, const_get) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
  const auto &const_d(d);
  auto view = const_d.get<const Data::Value>();
  // No non-const access to variable if Dataset is const, will not compile:
  // auto &view = const_d.get<Data::Value>();
  ASSERT_EQ(view.size(), 1);
  EXPECT_EQ(view[0], 1.1);
  // auto is deduced to be const, so assignment will not compile:
  // view[0] = 1.2;
}

TEST(Dataset, get) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
  auto view = d.get<Data::Value>();
  ASSERT_EQ(view.size(), 1);
  EXPECT_EQ(view[0], 1.1);
  view[0] = 2.2;
  EXPECT_EQ(view[0], 2.2);
}

TEST(Dataset, get_const) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Int>("name2", Dimensions{}, {2l});
  auto view = d.get<const Data::Value>();
  ASSERT_EQ(view.size(), 1);
  EXPECT_EQ(view[0], 1.1);
  // auto is now deduced to be const, so assignment will not compile:
  // view[0] = 1.2;
}

TEST(Dataset, get_fail) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Value>("name2", Dimensions{}, {1.1});
  EXPECT_THROW_MSG(d.get<Data::Value>(), std::runtime_error,
                   "Given variable tag is not unique. Must provide a name.");
  EXPECT_THROW_MSG(d.get<Data::Int>(), std::runtime_error,
                   "Dataset does not contain such a variable.");
}

TEST(Dataset, get_named) {
  Dataset d;
  d.insert<Data::Value>("name1", Dimensions{}, {1.1});
  d.insert<Data::Value>("name2", Dimensions{}, {2.2});
  auto var1 = d.get<Data::Value>("name1");
  ASSERT_EQ(var1.size(), 1);
  EXPECT_EQ(var1[0], 1.1);
  auto var2 = d.get<Data::Value>("name2");
  ASSERT_EQ(var2.size(), 1);
  EXPECT_EQ(var2[0], 2.2);
}

TEST(Dataset, operator_plus_equal) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>("name1", {Dimension::X, 1}, {2.2});
  a += a;
  EXPECT_EQ(a.get<Coord::X>()[0], 0.1);
  EXPECT_EQ(a.get<Data::Value>()[0], 4.4);
}

TEST(Dataset, operator_plus_equal_broadcast) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>(
      "name1",
      Dimensions({{Dimension::X, 1}, {Dimension::Y, 2}, {Dimension::Z, 3}}),
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  Dataset b;
  b.insert<Coord::X>({Dimension::X, 1}, {0.1});
  b.insert<Data::Value>("name1", Dimensions({{Dimension::Z, 3}}),
                        {0.1, 0.2, 0.3});

  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.get<Coord::X>()[0], 0.1);
  EXPECT_EQ(a.get<Data::Value>()[0], 1.1);
  EXPECT_EQ(a.get<Data::Value>()[1], 2.1);
  EXPECT_EQ(a.get<Data::Value>()[2], 3.2);
  EXPECT_EQ(a.get<Data::Value>()[3], 4.2);
  EXPECT_EQ(a.get<Data::Value>()[4], 5.3);
  EXPECT_EQ(a.get<Data::Value>()[5], 6.3);
}

TEST(Dataset, operator_plus_equal_transpose) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>(
      "name1",
      Dimensions({{Dimension::X, 1}, {Dimension::Y, 2}, {Dimension::Z, 3}}),
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  Dataset b;
  b.insert<Coord::X>({Dimension::X, 1}, {0.1});
  b.insert<Data::Value>("name1",
                        Dimensions({{Dimension::Z, 3}, {Dimension::Y, 2}}),
                        {0.1, 0.2, 0.3, 0.1, 0.2, 0.3});

  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.get<Coord::X>()[0], 0.1);
  EXPECT_EQ(a.get<Data::Value>()[0], 1.1);
  EXPECT_EQ(a.get<Data::Value>()[1], 2.1);
  EXPECT_EQ(a.get<Data::Value>()[2], 3.2);
  EXPECT_EQ(a.get<Data::Value>()[3], 4.2);
  EXPECT_EQ(a.get<Data::Value>()[4], 5.3);
  EXPECT_EQ(a.get<Data::Value>()[5], 6.3);
}

TEST(Dataset, operator_plus_equal_different_content) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>("name1", {Dimension::X, 1}, {2.2});
  Dataset b;
  b.insert<Coord::X>({Dimension::X, 1}, {0.1});
  b.insert<Data::Value>("name1", {Dimension::X, 1}, {2.2});
  b.insert<Data::Value>("name2", {Dimension::X, 1}, {3.3});
  EXPECT_THROW_MSG(a += b, std::runtime_error, "Right-hand-side in addition "
                                               "contains variable that is not "
                                               "present in left-hand-side.");
  EXPECT_NO_THROW(b += a);
}

TEST(Dataset, operator_plus_equal_history) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>("name1", {Dimension::X, 1}, {2.2});
  a.insert<Data::History>("history", {}, 1);
  a += a;
  EXPECT_EQ(a.get<Coord::X>()[0], 0.1);
  EXPECT_EQ(a.get<Data::Value>()[0], 4.4);
  EXPECT_EQ(a.get<Data::History>()[0].size(), 1);
  EXPECT_EQ(a.get<Data::History>()[0][0], "operator+=");
  a += a;
  EXPECT_EQ(a.get<Data::History>()[0].size(), 2);
  EXPECT_EQ(a.get<Data::History>()[0][1], "operator+=");
}

TEST(Dataset, history_with_slicing) {
  Dataset d;
  d.insert<Coord::X>({Dimension::X, 3}, {0.1, 0.2, 0.3});
  d.insert<Data::Value>("name", {Dimension::X, 3}, {1.1, 2.2, 3.3});
  d.insert<Data::History>("history", {}, 1);
  d += d;
  for (gsl::index i = 0; i < 3; ++i) {
    auto s = slice(d, Dimension::X, i);
    // By default, slicing operations should probably also write to history.
    // However, for the purpose of using slicing for cache blocking we do not
    // want a polluted history. One option is the remove the history from all by
    // one slice before processing. There may be the need for additional cleanup
    // of the history after calling Dataset::setSlice (in the current
    // implementation it does not write to the history, so there is no issue).
    // The described history management would be part of a class/function
    // performing the blocking and would not be exposed to the user.
    if (i != 0)
      s.erase<Data::History>();
    s += s;
    d.setSlice(s, Dimension::X, i);
  }
  EXPECT_EQ(d.get<Data::History>()[0].size(), 2);
  EXPECT_EQ(d.get<Data::History>()[0][0], "operator+=");
  EXPECT_EQ(d.get<Data::History>()[0][1], "operator+=");
}

TEST(Dataset, operator_times_equal) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>("name1", {Dimension::X, 1}, {3.0});
  a *= a;
  EXPECT_EQ(a.get<Coord::X>()[0], 0.1);
  EXPECT_EQ(a.get<Data::Value>()[0], 9.0);
}

TEST(Dataset, operator_times_equal_with_uncertainty) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>("name1", {Dimension::X, 1}, {3.0});
  a.insert<Data::Variance>("name1", {Dimension::X, 1}, {2.0});
  Dataset b;
  b.insert<Coord::X>({Dimension::X, 1}, {0.1});
  b.insert<Data::Value>("name1", {Dimension::X, 1}, {4.0});
  b.insert<Data::Variance>("name1", {Dimension::X, 1}, {3.0});
  a *= b;
  EXPECT_EQ(a.get<Coord::X>()[0], 0.1);
  EXPECT_EQ(a.get<Data::Value>()[0], 12.0);
  EXPECT_EQ(a.get<Data::Variance>()[0], 2.0 * 16.0 + 3.0 * 9.0);
}

TEST(Dataset, operator_times_equal_uncertainty_failures) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>("name1", {Dimension::X, 1}, {3.0});
  a.insert<Data::Variance>("name1", {Dimension::X, 1}, {2.0});
  Dataset b;
  b.insert<Coord::X>({Dimension::X, 1}, {0.1});
  b.insert<Data::Value>("name1", {Dimension::X, 1}, {4.0});
  Dataset c;
  c.insert<Coord::X>({Dimension::X, 1}, {0.1});
  c.insert<Data::Variance>("name1", {Dimension::X, 1}, {2.0});
  EXPECT_THROW_MSG(a *= b, std::runtime_error, "Either both or none of the "
                                               "operands must have a variance "
                                               "for their values.");
  EXPECT_THROW_MSG(b *= a, std::runtime_error, "Either both or none of the "
                                               "operands must have a variance "
                                               "for their values.");
  EXPECT_THROW_MSG(c *= c, std::runtime_error, "Cannot multiply datasets that "
                                               "contain a variance but no "
                                               "corresponding value.");
  EXPECT_THROW_MSG(a *= c, std::runtime_error, "Cannot multiply datasets that "
                                               "contain a variance but no "
                                               "corresponding value.");
  EXPECT_THROW_MSG(c *= a, std::runtime_error, "Right-hand-side in addition "
                                               "contains variable that is not "
                                               "present in left-hand-side.");
  EXPECT_THROW_MSG(b *= c, std::runtime_error, "Right-hand-side in addition "
                                               "contains variable that is not "
                                               "present in left-hand-side.");
  EXPECT_THROW_MSG(c *= b, std::runtime_error, "Right-hand-side in addition "
                                               "contains variable that is not "
                                               "present in left-hand-side.");
}

TEST(Dataset, operator_times_equal_with_units) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  auto values =
      makeVariable<Data::Value>(Dimensions({{Dimension::X, 1}}), {3.0});
  values.setName("name1");
  values.setUnit(Unit::Id::Length);
  auto variances =
      makeVariable<Data::Variance>(Dimensions({{Dimension::X, 1}}), {2.0});
  variances.setName("name1");
  variances.setUnit(Unit::Id::Area);
  a.insert(values);
  a.insert(variances);
  a *= a;
  EXPECT_EQ(a.unit<Data::Value>(), Unit::Id::Area);
  EXPECT_EQ(a.unit<Data::Variance>(), Unit::Id::AreaVariance);
  EXPECT_EQ(a.get<Data::Variance>()[0], 36.0);
}

TEST(Dataset, operator_times_equal_histogram_data) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  auto values =
      makeVariable<Data::Value>(Dimensions({{Dimension::X, 1}}), {3.0});
  values.setName("name1");
  values.setUnit(Unit::Id::Counts);
  auto variances =
      makeVariable<Data::Variance>(Dimensions({{Dimension::X, 1}}), {2.0});
  variances.setName("name1");
  variances.setUnit(Unit::Id::CountsVariance);
  a.insert(values);
  a.insert(variances);

  Dataset b;
  b.insert<Coord::X>({Dimension::X, 1}, {0.1});
  b.insert<Data::Value>("name1", {Dimension::X, 1}, {4.0});
  b.insert<Data::Variance>("name1", {Dimension::X, 1}, {4.0});

  // Counts (aka "histogram data") times counts not possible.
  EXPECT_THROW_MSG(a *= a, std::runtime_error, "Unsupported unit on LHS");
  // Counts times frequencies (aka "distribution") ok.
  // TODO Works for dimensionless right now, but do we need to handle other
  // cases as well?
  auto a_copy(a);
  EXPECT_NO_THROW(a *= b);
  EXPECT_NO_THROW(b *= a_copy);
}

TEST(Dataset, operator_plus_with_temporary_avoids_copy) {
  Dataset a;
  a.insert<Data::Value>("name", {Dimension::X, 1}, {2.2});
  const auto a2(a);
  const auto b(a);

  const auto *addr = &a.get<Data::Value>()[0];
  auto sum = std::move(a) + b;
  EXPECT_EQ(&sum.get<Data::Value>()[0], addr);

  const auto *addr2 = &a2.get<Data::Value>()[0];
  auto sum2 = a2 + b;
  EXPECT_NE(&sum2.get<Data::Value>()[0], addr2);
}

TEST(Dataset, slice) {
  Dataset d;
  d.insert<Coord::X>({Dimension::X, 2}, {0.0, 0.1});
  d.insert<Data::Value>("data",
                        Dimensions({{Dimension::X, 2}, {Dimension::Y, 3}}),
                        {0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
  for (const gsl::index i : {0, 1}) {
    auto sliceX = slice(d, Dimension::X, i);
    ASSERT_EQ(sliceX.size(), 2);
    ASSERT_EQ(sliceX.get<const Coord::X>().size(), 1);
    EXPECT_EQ(sliceX.get<const Coord::X>()[0], 0.1 * i);
    ASSERT_EQ(sliceX.get<const Data::Value>().size(), 3);
    EXPECT_EQ(sliceX.get<const Data::Value>()[0], 0.0 + i);
    EXPECT_EQ(sliceX.get<const Data::Value>()[1], 2.0 + i);
    EXPECT_EQ(sliceX.get<const Data::Value>()[2], 4.0 + i);
  }
  for (const gsl::index i : {0, 1, 2}) {
    auto sliceY = slice(d, Dimension::Y, i);
    ASSERT_EQ(sliceY.size(), 2);
    ASSERT_EQ(sliceY.get<const Coord::X>(), d.get<const Coord::X>());
    ASSERT_EQ(sliceY.get<const Data::Value>().size(), 2);
    EXPECT_EQ(sliceY.get<const Data::Value>()[0], 0.0 + 2 * i);
    EXPECT_EQ(sliceY.get<const Data::Value>()[1], 1.0 + 2 * i);
  }
  EXPECT_NO_THROW(slice(d, Dimension::Z, 0));
  EXPECT_THROW_MSG(slice(d, Dimension::Z, 1), std::runtime_error,
                   "Slice index out of range");
}

TEST(Dataset, concatenate_constant_dimension_broken) {
  Dataset a;
  a.insert<Data::Value>("name1", Dimensions{}, {1.1});
  a.insert<Data::Value>("name2", Dimensions{}, {2.2});
  auto d = concatenate(Dimension::X, a, a);
  // TODO Special case: No variable depends on X so the result does not contain
  // this dimension either. Change this behavior?!
  EXPECT_FALSE(d.dimensions().contains(Dimension::X));
}

TEST(Dataset, concatenate) {
  Dataset a;
  a.insert<Coord::X>({Dimension::X, 1}, {0.1});
  a.insert<Data::Value>("data", {Dimension::X, 1}, {2.2});
  auto x = concatenate(Dimension::X, a, a);
  EXPECT_TRUE(x.dimensions().contains(Dimension::X));
  EXPECT_EQ(x.get<const Coord::X>().size(), 2);
  EXPECT_EQ(x.get<const Data::Value>().size(), 2);
  auto x2(x);
  x2.get<Data::Value>()[0] = 100.0;
  auto xy = concatenate(Dimension::Y, x, x2);
  EXPECT_TRUE(xy.dimensions().contains(Dimension::X));
  EXPECT_TRUE(xy.dimensions().contains(Dimension::Y));
  EXPECT_EQ(xy.get<const Coord::X>().size(), 2);
  EXPECT_EQ(xy.get<const Data::Value>().size(), 4);
  // Coord::X is shared since it it was the same in x and x2 and is thus
  // "constant" along Dimension::Y in xy.
  EXPECT_EQ(&x.get<const Coord::X>()[0], &xy.get<const Coord::X>()[0]);
  EXPECT_NE(&x.get<const Data::Value>()[0], &xy.get<const Data::Value>()[0]);

  xy = concatenate(Dimension::Y, xy, x);
  EXPECT_EQ(xy.get<const Coord::X>().size(), 2);
  EXPECT_EQ(xy.get<const Data::Value>().size(), 6);

  xy = concatenate(Dimension::Y, xy, xy);
  EXPECT_EQ(xy.get<const Coord::X>().size(), 2);
  EXPECT_EQ(xy.get<const Data::Value>().size(), 12);
}
