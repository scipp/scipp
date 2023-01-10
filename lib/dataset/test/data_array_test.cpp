// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/data_array.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/to_unit.h"

#include "test_macros.h"

using namespace scipp;

class DataArrayTest : public ::testing::Test {
protected:
  Variable data = makeVariable<double>(Values{1});
  Variable coord = makeVariable<double>(Values{2});
  Variable mask = makeVariable<bool>(Values{false});
  Variable attr = makeVariable<double>(Values{3});
};

TEST_F(DataArrayTest, constructor_shares) {
  DataArray a(data, {{Dim::X, coord}}, {{"mask", mask}}, {{Dim("attr"), attr}});
  EXPECT_TRUE(a.data().is_same(data));
  EXPECT_TRUE(a.coords()[Dim::X].is_same(coord));
  EXPECT_TRUE(a.masks()["mask"].is_same(mask));
  EXPECT_TRUE(a.attrs()[Dim("attr")].is_same(attr));
}

TEST_F(DataArrayTest, copy_shares) {
  const DataArray a(data, {{Dim::X, coord}}, {{"mask", mask}},
                    {{Dim("attr"), attr}});
  const DataArray b(a);
  EXPECT_TRUE(a.data().is_same(b.data()));
  EXPECT_TRUE(a.coords()[Dim::X].is_same(b.coords()[Dim::X]));
  EXPECT_TRUE(a.masks()["mask"].is_same(b.masks()["mask"]));
  EXPECT_TRUE(a.attrs()[Dim("attr")].is_same(b.attrs()[Dim("attr")]));
  // Meta data may be shallow-copied but dicts are not shared
  EXPECT_NE(&a.coords(), &b.coords());
  EXPECT_NE(&a.masks(), &b.masks());
  EXPECT_NE(&a.attrs(), &b.attrs());
}

TEST_F(DataArrayTest, copy_assign_shares) {
  DataArray a(data, {{Dim::X, coord}}, {{"mask", mask}}, {{Dim("attr"), attr}});
  DataArray b{coord};
  b = a;
  EXPECT_TRUE(a.data().is_same(b.data()));
  EXPECT_TRUE(a.coords()[Dim::X].is_same(b.coords()[Dim::X]));
  EXPECT_TRUE(a.masks()["mask"].is_same(b.masks()["mask"]));
  EXPECT_TRUE(a.attrs()[Dim("attr")].is_same(b.attrs()[Dim("attr")]));
  // Meta data may be shallow-copied but dicts are not shared
  EXPECT_NE(&a.coords(), &b.coords());
  EXPECT_NE(&a.masks(), &b.masks());
  EXPECT_NE(&a.attrs(), &b.attrs());
}

TEST_F(DataArrayTest, construct_fail) {
  // Invalid data
  EXPECT_THROW(DataArray(Variable{}), std::runtime_error);
}

TEST_F(DataArrayTest, name) {
  DataArray array(data);
  EXPECT_EQ(array.name(), "");
  array.setName("newname");
  EXPECT_EQ(array.name(), "newname");
}

TEST_F(DataArrayTest, get_coord) {
  DataArray a(data);
  a.coords().set(Dim::X, coord);
  EXPECT_EQ(a.coords().at(Dim::X), coord);
  EXPECT_THROW_DISCARD(a.attrs().at(Dim::X), except::NotFoundError);
  EXPECT_THROW_DISCARD(a.coords().at(Dim::Y), except::NotFoundError);
}

TEST_F(DataArrayTest, erase_coord) {
  DataArray a(data);
  a.coords().set(Dim::X, coord);
  EXPECT_THROW(a.attrs().erase(Dim::X), except::NotFoundError);
  EXPECT_NO_THROW(a.coords().erase(Dim::X));
  a.attrs().set(Dim::X, attr);
  EXPECT_NO_THROW(a.attrs().erase(Dim::X));
  a.attrs().set(Dim::X, attr);
  EXPECT_THROW(a.coords().erase(Dim::X), except::NotFoundError);
}

TEST_F(DataArrayTest, shadow_attr) {
  const auto var1 = 1.0 * units::m;
  const auto var2 = 2.0 * units::m;
  DataArray a(0.0 * units::m);
  a.coords().set(Dim::X, var1);
  a.attrs().set(Dim::X, var2);
  EXPECT_EQ(a.coords()[Dim::X], var1);
  EXPECT_EQ(a.attrs()[Dim::X], var2);
  EXPECT_THROW_DISCARD(a.meta(), except::DataArrayError);
  a.attrs().erase(Dim::X);
  EXPECT_EQ(a.meta()[Dim::X], var1);
}

TEST_F(DataArrayTest, mutate_via_meta_throws) {
  DataArray da(data);
  da.coords().set(Dim::X, coord);
  const auto original = copy(da);
  EXPECT_THROW(da.meta().erase(Dim::X), except::DataArrayError);
  EXPECT_EQ(da, original);
  EXPECT_THROW_DISCARD(da.meta().extract(Dim::X), except::DataArrayError);
  EXPECT_EQ(da, original);
  EXPECT_THROW(da.meta().set(Dim::Y, coord), except::DataArrayError);
  EXPECT_EQ(da, original);
}

TEST_F(DataArrayTest, view) {
  const auto var = makeVariable<double>(Values{1});
  const DataArray a(copy(var), {{Dim::X, copy(var)}}, {{"mask", copy(var)}},
                    {{Dim("attr"), copy(var)}});
  const auto b = a.view();
  EXPECT_EQ(a, b);
  EXPECT_EQ(&a.data(), &b.data());
  EXPECT_EQ(&a.coords(), &b.coords());
  EXPECT_EQ(&a.masks(), &b.masks());
  EXPECT_EQ(&a.attrs(), &b.attrs());
  EXPECT_EQ(a.name(), b.name());
}

TEST_F(DataArrayTest, as_const) {
  const auto var = makeVariable<double>(Values{1});
  const DataArray a(copy(var), {{Dim::X, copy(var)}}, {{"mask", copy(var)}},
                    {{Dim("attr"), copy(var)}});
  EXPECT_FALSE(var.is_readonly());
  const auto b = a.as_const();
  EXPECT_EQ(a, b);
  EXPECT_TRUE(b.is_readonly());
  EXPECT_TRUE(b.coords().is_readonly());
  EXPECT_TRUE(b.masks().is_readonly());
  EXPECT_TRUE(b.attrs().is_readonly());
  EXPECT_TRUE(b.coords()[Dim::X].is_readonly());
  EXPECT_TRUE(b.masks()["mask"].is_readonly());
  EXPECT_TRUE(b.attrs()[Dim("attr")].is_readonly());
  EXPECT_EQ(a.name(), b.name());
}

TEST_F(DataArrayTest, full_slice) {
  DataArray a(data, {{Dim::X, coord}}, {{"mask", mask}}, {{Dim("attr"), attr}});
  const auto slice = a.slice({});
  EXPECT_TRUE(slice.data().is_same(a.data()));
  EXPECT_TRUE(slice.coords()[Dim::X].is_same(a.coords()[Dim::X]));
  EXPECT_TRUE(slice.masks()["mask"].is_same(a.masks()["mask"]));
  EXPECT_TRUE(slice.attrs()[Dim("attr")].is_same(a.attrs()[Dim("attr")]));
}

TEST_F(DataArrayTest, self_nesting) {
  DataArray inner{makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2})};
  Variable var = makeVariable<DataArray>(Values{inner});

  DataArray nested_in_data{var};
  ASSERT_THROW_DISCARD(var.value<DataArray>() = nested_in_data,
                       std::invalid_argument);

  DataArray nested_in_coord{
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{3, 4})};
  nested_in_coord.coords().set(Dim::X, var);
  ASSERT_THROW_DISCARD(var.value<DataArray>() = nested_in_coord,
                       std::invalid_argument);

  DataArray nested_in_mask{
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{3, 4})};
  nested_in_coord.masks().set("mask", var);
  ASSERT_THROW_DISCARD(var.value<DataArray>() = nested_in_coord,
                       std::invalid_argument);

  DataArray nested_in_attr{
      makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{3, 4})};
  nested_in_coord.attrs().set(Dim::X, var);
  ASSERT_THROW_DISCARD(var.value<DataArray>() = nested_in_coord,
                       std::invalid_argument);
}

TEST_F(DataArrayTest, is_edges_1d_without_edges) {
  DataArray da{
      makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{1, 2}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{0, 1})}}};
  ASSERT_FALSE(da.coords().is_edges(Dim::X));
  ASSERT_FALSE(da.coords().is_edges(Dim::X, Dim::X));
}

TEST_F(DataArrayTest, is_edges_1d_with_edges) {
  DataArray da{
      makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{1, 2}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{0, 1, 2})}}};
  ASSERT_TRUE(da.coords().is_edges(Dim::X));
  ASSERT_TRUE(da.coords().is_edges(Dim::X, Dim::X));
}

TEST_F(DataArrayTest, is_edges_2d_with_edges) {
  DataArray da{
      makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                        Values{1, 2, 3, 4, 5, 6}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{0, 1, 2})},
       {Dim::Y, makeVariable<int>(Dims{Dim::Y}, Shape{3}, Values{0, 1, 2})}}};
  ASSERT_TRUE(da.coords().is_edges(Dim::X));
  ASSERT_TRUE(da.coords().is_edges(Dim::X, Dim::X));
  ASSERT_FALSE(da.coords().is_edges(Dim::Y));
  ASSERT_FALSE(da.coords().is_edges(Dim::Y, Dim::Y));
}

TEST_F(DataArrayTest, is_edges_2d_with_2d_edges) {
  DataArray da{
      makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                        Values{1, 2, 3, 4, 5, 6}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{3, 3},
                                  Values{0, 1, 2, 3, 4, 5, 6, 7, 8})}}};
  ASSERT_TRUE(da.coords().is_edges(Dim::X, Dim::X));
  ASSERT_FALSE(da.coords().is_edges(Dim::X, Dim::Y));
}

TEST_F(DataArrayTest, is_edges_2d_coord_requires_dim_parameter) {
  DataArray da{
      makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{2, 3},
                        Values{1, 2, 3, 4, 5, 6}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{3, 3},
                                  Values{0, 1, 2, 3, 4, 5, 6, 7, 8})}}};
  ASSERT_THROW_DISCARD(da.coords().is_edges(Dim::X), except::DimensionError);
}

TEST_F(DataArrayTest, is_edges_scalar_with_edges) {
  DataArray da{
      makeVariable<int>(Dims{}, Shape{}, Values{1}),
      {{Dim{"c"}, makeVariable<int>(Dims{Dim::Y}, Shape{2}, Values{0, 1})}}};
  ASSERT_TRUE(da.coords().is_edges(Dim{"c"}));
  ASSERT_TRUE(da.coords().is_edges(Dim{"c"}, Dim::Y));
}

TEST_F(DataArrayTest, is_edges_invalid_coord_name_throws_NotFoundError) {
  DataArray da{
      makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{1, 2}),
      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3}, Values{0, 1, 2})}}};
  ASSERT_THROW_DISCARD(da.coords().is_edges(Dim{"invalid name"}),
                       except::NotFoundError);
}

DataArray make_drop_example_data_arrays() {
  Variable data = makeVariable<double>(Values{1});
  Variable coord = makeVariable<double>(Values{2});
  Variable mask = makeVariable<bool>(Values{false});
  Variable attr = makeVariable<double>(Values{3});
  return DataArray(
      data, {{Dim{"dim0"}, coord}, {Dim{"dim1"}, coord}, {Dim{"dim2"}, coord}},
      {{"mask0", mask}, {"mask1", mask}, {"mask2", mask}},
      {{Dim{"attr0"}, attr}, {Dim{"attr1"}, attr}, {Dim{"attr2"}, attr}});
}

TEST_F(DataArrayTest, drop_single_coord) {
  const DataArray da = make_drop_example_data_arrays();
  auto new_da = da.drop_coords(std::vector{Dim{"dim0"}});
  const DataArray expected_da{
      data,
      {{Dim{"dim1"}, coord}, {Dim{"dim2"}, coord}},
      {{"mask0", mask}, {"mask1", mask}, {"mask2", mask}},
      {{Dim{"attr0"}, attr}, {Dim{"attr1"}, attr}, {Dim{"attr2"}, attr}}};
  ASSERT_EQ(new_da, expected_da);
}

TEST_F(DataArrayTest, drop_multiple_coords) {
  const DataArray da = make_drop_example_data_arrays();
  auto new_da = da.drop_coords(std::vector{Dim{"dim1"}, Dim{"dim2"}});
  const DataArray expected_da{
      data,
      {{Dim{"dim0"}, coord}},
      {{"mask0", mask}, {"mask1", mask}, {"mask2", mask}},
      {{Dim{"attr0"}, attr}, {Dim{"attr1"}, attr}, {Dim{"attr2"}, attr}}};
  ASSERT_EQ(new_da, expected_da);
}

TEST_F(DataArrayTest, drop_single_mask) {
  const DataArray da = make_drop_example_data_arrays();
  auto new_da = da.drop_masks({{"mask0"}});
  const DataArray expected_da{
      data,
      {{Dim{"dim0"}, coord}, {Dim{"dim1"}, coord}, {Dim{"dim2"}, coord}},
      {{"mask1", mask}, {"mask2", mask}},
      {{Dim{"attr0"}, attr}, {Dim{"attr1"}, attr}, {Dim{"attr2"}, attr}}};
  ASSERT_EQ(new_da, expected_da);
}

TEST_F(DataArrayTest, drop_multiple_mask) {
  const DataArray da = make_drop_example_data_arrays();
  auto new_da = da.drop_masks({{"mask2", "mask1"}});
  const DataArray expected_da{
      data,
      {{Dim{"dim0"}, coord}, {Dim{"dim1"}, coord}, {Dim{"dim2"}, coord}},
      {{"mask0", mask}},
      {{Dim{"attr0"}, attr}, {Dim{"attr1"}, attr}, {Dim{"attr2"}, attr}}};
  ASSERT_EQ(new_da, expected_da);
}

TEST_F(DataArrayTest, drop_single_attrs) {
  const DataArray da = make_drop_example_data_arrays();
  auto new_da = da.drop_attrs(std::vector{Dim{"attr0"}});
  const DataArray expected_da{
      data,
      {{Dim{"dim0"}, coord}, {Dim{"dim1"}, coord}, {Dim{"dim2"}, coord}},
      {{"mask0", mask}, {"mask1", mask}, {"mask2", mask}},
      {{Dim{"attr1"}, attr}, {Dim{"attr2"}, attr}}};
  ASSERT_EQ(new_da, expected_da);
}

TEST_F(DataArrayTest, drop_multiple_attrs) {
  const DataArray da = make_drop_example_data_arrays();
  auto new_da = da.drop_attrs(std::vector{Dim{"attr1"}, Dim{"attr2"}});
  const DataArray expected_da{
      data,
      {{Dim{"dim0"}, coord}, {Dim{"dim1"}, coord}, {Dim{"dim2"}, coord}},
      {{"mask0", mask}, {"mask1", mask}, {"mask2", mask}},
      {{Dim{"attr0"}, attr}}};
  ASSERT_EQ(new_da, expected_da);
}
