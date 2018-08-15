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
  auto edges = makeDataArray<Data::Error>(Dimensions(Dimension::Tof, 3),
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
      makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  EXPECT_THROW_MSG(
      d.insertAsEdge(Dimension::Tof, too_short), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
  auto too_long = makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 4),
                                             {1.1, 2.2, 3.3, 4.4});
  EXPECT_THROW_MSG(
      d.insertAsEdge(Dimension::Tof, too_long), std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
  auto edges = makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 3),
                                          {1.1, 2.2, 3.3});
  EXPECT_THROW_MSG(d.insertAsEdge(Dimension::X, edges), std::runtime_error,
                   "Dimension not found.");
}

TEST(Dataset, insertAsEdge_reverse_fail) {
  Dataset d;
  auto edges =
      makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  d.insertAsEdge(Dimension::Tof, edges);
  EXPECT_THROW_MSG(
      d.insert<Data::Value>("name1", Dimensions(Dimension::Tof, 2), {1.1, 2.2}),
      std::runtime_error,
      "Cannot insert variable into Dataset: Dimensions do not match");
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
