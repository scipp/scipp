// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "scipp/core/except.h"
#include "test_macros.h"
#include <gtest/gtest-matchers.h>
#include <gtest/gtest.h>

#include <numeric>
#include <set>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

#include "dataset_test_common.h"
#include "test_data_arrays.h"
#include "test_macros.h"

using namespace scipp;

// Any dataset functionality that is also available for Dataset(Const)View is
// to be tested in dataset_view_test.cpp, not here!

TEST(DatasetTest, construct_default) {
  Dataset d;
  ASSERT_FALSE(d.is_valid());
}

TEST(DatasetTest, construct_empty_dicts) {
  Dataset d({}, {});
  ASSERT_EQ(d.ndim(), 0);
}

TEST(DatasetTest, construct_data) {
  const Dataset d(
      {{"a", DataArray(makeVariable<int>(Dims{Dim::X}, Shape{3}))}});
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 3}}));
  ASSERT_EQ(d["a"], DataArray(makeVariable<int>(Dims{Dim::X}, Shape{3})));
  ASSERT_TRUE(d.coords().empty());
  ASSERT_TRUE(d.is_valid());
}

TEST(DatasetTest, construct_multiple_conflicting_data) {
  ASSERT_THROW(
      Dataset({{"a", DataArray(makeVariable<int>(Dims{Dim::X}, Shape{4}))},
               {"b", DataArray(makeVariable<int>(Dims{Dim::X}, Shape{5}))}}),
      except::DimensionError);
}

TEST(DatasetTest, construct_multiple_conflicting_data_extra_dim) {
  ASSERT_THROW(
      Dataset({{"a", DataArray(makeVariable<int>(Dims{Dim::X}, Shape{4}))},
               {"b", DataArray(makeVariable<int>(Dims{Dim::Y}, Shape{5}))}}),
      except::DimensionError);
}

TEST(DatasetTest, construct_coord) {
  const Dataset d({}, {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{4})}});
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 4}}));
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.coords()[Dim::X], makeVariable<int>(Dims{Dim::X}, Shape{4}));
  ASSERT_TRUE(d.is_valid());
}

TEST(DatasetTest, construct_multiple_coords_grow_dimension) {
  const Dataset d(
      {}, {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{4})},
           {Dim::Y, makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{4, 6})},
           {Dim::Z, makeVariable<int>(Dims{Dim::Z}, Shape{2})}});
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 4}, {Dim::Y, 6}, {Dim::Z, 2}}));
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.coords()[Dim::X], makeVariable<int>(Dims{Dim::X}, Shape{4}));
  ASSERT_EQ(d.coords()[Dim::Y],
            makeVariable<int>(Dims{Dim::X, Dim::Y}, Shape{4, 6}));
  ASSERT_EQ(d.coords()[Dim::Z], makeVariable<int>(Dims{Dim::Z}, Shape{2}));
}

TEST(DatasetTest, construct_multiple_coords_bin_edges_second) {
  const Dataset d({}, {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{4})},
                       {Dim::Y, makeVariable<int>(Dims{Dim::X}, Shape{5})}});
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 4}}));
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.coords()[Dim::X], makeVariable<int>(Dims{Dim::X}, Shape{4}));
  ASSERT_EQ(d.coords()[Dim::Y], makeVariable<int>(Dims{Dim::X}, Shape{5}));
}

TEST(DatasetTest, construct_multiple_coords_bin_edges_first) {
  const Dataset d({}, {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{5})},
                       {Dim::Y, makeVariable<int>(Dims{Dim::X}, Shape{4})}});
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 4}}));
  ASSERT_TRUE(d.empty());
  ASSERT_EQ(d.coords()[Dim::X], makeVariable<int>(Dims{Dim::X}, Shape{5}));
  ASSERT_EQ(d.coords()[Dim::Y], makeVariable<int>(Dims{Dim::X}, Shape{4}));
}

TEST(DatasetTest, construct_multiple_conflicting_coords) {
  ASSERT_THROW(
      Dataset({}, {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{4})},
                   {Dim::Y, makeVariable<int>(Dims{Dim::X}, Shape{6})}}),
      except::DimensionError);
}

TEST(DatasetTest, construct_data_and_coord) {
  const Dataset d({{"a", DataArray(makeVariable<int>(Dims{Dim::X}, Shape{5}))}},
                  {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{5})}});
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 5}}));
  ASSERT_EQ(d["a"],
            DataArray(makeVariable<int>(Dims{Dim::X}, Shape{5}),
                      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{5})}}));
  ASSERT_EQ(d.coords()[Dim::X], makeVariable<int>(Dims{Dim::X}, Shape{5}));
}

TEST(DatasetTest, construct_data_and_bin_edge_coord) {
  const Dataset d({{"a", DataArray(makeVariable<int>(Dims{Dim::X}, Shape{5}))}},
                  {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{6})}});
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 5}}));
  ASSERT_EQ(d["a"],
            DataArray(makeVariable<int>(Dims{Dim::X}, Shape{5}),
                      {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{6})}}));
  ASSERT_EQ(d.coords()[Dim::X], makeVariable<int>(Dims{Dim::X}, Shape{6}));
}

TEST(DatasetTest, construct_conflicting_data_and_coord) {
  ASSERT_THROW(
      Dataset({{"a", DataArray(makeVariable<int>(Dims{Dim::X}, Shape{4}))}},
              {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{6})}}),
      except::DimensionError);
}

TEST(DatasetTest, assign_copy) {
  const Dataset a({{"a", makeVariable<double>(Dims{Dim::X}, Shape{4})}},
                  {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{4})}});
  Dataset b;
  b = a;
  ASSERT_EQ(a, b);
  ASSERT_TRUE(b.is_valid());
}

TEST(DatasetTest, clear) {
  DatasetFactory factory({{Dim::X, 2}, {Dim::Y, 3}});
  auto dataset = factory.make("data");

  ASSERT_FALSE(dataset.empty());
  ASSERT_FALSE(dataset.coords().empty());
  ASSERT_EQ(dataset.dims(), Dimensions({{Dim::X, 2}, {Dim::Y, 3}}));

  ASSERT_NO_THROW(dataset.clear());

  ASSERT_TRUE(dataset.empty());
  ASSERT_FALSE(dataset.coords().empty());
  ASSERT_EQ(dataset.dims(), Dimensions({{Dim::X, 2}, {Dim::Y, 3}}));
}

TEST(DatasetTest, can_set_coord_after_clear) {
  DatasetFactory factory({{Dim::X, 2}, {Dim::Y, 3}});
  auto dataset = factory.make("data");
  dataset.clear();

  ASSERT_NO_THROW(dataset.setCoord(
      Dim{"new x"}, makeVariable<double>(Dims{Dim::X}, Shape{2})));
}

TEST(DatasetTest, erase_non_existent) {
  DatasetFactory factory;
  auto dataset = factory.make("data");
  ASSERT_THROW(dataset.erase("not an item"), except::NotFoundError);
  ASSERT_THROW_DISCARD(dataset.extract("not an item"), except::NotFoundError);
}

TEST(DatasetTest, erase) {
  DatasetFactory factory;
  auto dataset = factory.make("data");
  ASSERT_NO_THROW(dataset.erase("data"));
  ASSERT_FALSE(dataset.contains("data"));
}

TEST(DatasetTest, erase_preserves_coords) {
  DatasetFactory factory;
  auto dataset = factory.make("data");
  const Dataset original = dataset;
  dataset.erase("data");
  ASSERT_FALSE(dataset.coords().empty());
  ASSERT_EQ(dataset.coords(), original.coords());
}

TEST(DatasetTest, extract) {
  DatasetFactory factory;
  auto dataset = factory.make("data");
  auto reference = copy(dataset);

  auto ptr = dataset["data"].values<double>().data();
  auto array = dataset.extract("data");
  EXPECT_EQ(array.values<double>().data(), ptr);

  ASSERT_FALSE(dataset.contains("data"));
  EXPECT_EQ(array, reference["data"]);
  reference.erase("data");
  EXPECT_EQ(dataset, reference);
}

TEST(DatasetTest, extract_preserves_coords) {
  DatasetFactory factory;
  auto dataset = factory.make("data");
  const Dataset original = dataset;
  [[maybe_unused]] const auto x = dataset.extract("data");
  ASSERT_FALSE(dataset.coords().empty());
  ASSERT_EQ(dataset.coords(), original.coords());
}

TEST(DatasetTest, cannot_reshape_after_erase) {
  DatasetFactory factory({{Dim::X, 10}});
  auto d = factory.make("a");

  ASSERT_TRUE(d.contains("a"));

  ASSERT_NO_THROW(d.erase("a"));
  ASSERT_FALSE(d.contains("a"));

  ASSERT_THROW(d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{15})),
               scipp::except::DimensionError);
  ASSERT_FALSE(d.contains("a"));
}

TEST(DatasetTest, cannot_reshape_after_extract) {
  DatasetFactory factory({{Dim::X, 10}});
  auto d = factory.make("a");

  ASSERT_TRUE(d.contains("a"));

  ASSERT_NO_THROW(auto _ = d.extract("a"));
  ASSERT_FALSE(d.contains("a"));

  ASSERT_THROW(d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{15})),
               scipp::except::DimensionError);
  ASSERT_FALSE(d.contains("a"));
}

TEST(DatasetTest, dim_0d) {
  const Dataset d({{"a", makeVariable<double>()}});
  ASSERT_THROW(d.dim(), except::DimensionError);
}

TEST(DatasetTest, dim_1d) {
  const Dataset d({{"a", makeVariable<double>(Dims{Dim::X}, Shape{2})}});
  ASSERT_EQ(d.dim(), Dim::X);
}

TEST(DatasetTest, dim_2d) {
  const Dataset d(
      {{"a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3})}});
  ASSERT_THROW(d.dim(), except::DimensionError);
}

TEST(DatasetTest, erase_preserves_dim) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X}, Shape{2})}});
  d.erase("a");
  ASSERT_EQ(d.dim(), Dim::X);
}

TEST(DatasetTest, setCoord) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  Dataset d({{"a", var}});

  ASSERT_EQ(d.size(), 1);
  ASSERT_EQ(d.coords().size(), 0);
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 3}}));

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 1);
  ASSERT_EQ(d.coords().size(), 1);
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 3}}));

  ASSERT_NO_THROW(d.setCoord(Dim::Y, var));
  ASSERT_EQ(d.size(), 1);
  ASSERT_EQ(d.coords().size(), 2);
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 3}}));

  ASSERT_NO_THROW(d.setCoord(Dim::X, var));
  ASSERT_EQ(d.size(), 1);
  ASSERT_EQ(d.coords().size(), 2);
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 3}}));
}

TEST(DatasetTest, setCoord_length_2) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X}, Shape{2})}});
  d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{2}));
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 2}}));
}

TEST(DatasetTest, setCoord_length_2_bin_edges) {
  Dataset d({{"a", makeVariable<double>(Dims{})}});
  d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{2}));
  ASSERT_EQ(d.dims(), Dimensions());
}

TEST(DatasetTest, setCoord_length_2_multi_dim) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3})}});
  d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3}));
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 2}, {Dim::Y, 3}}));
}

TEST(DatasetTest, setCoord_length_2_multi_dim_bin_edges) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::Y}, Shape{3})}});
  d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3}));
  ASSERT_EQ(d.dims(), Dimensions({{Dim::Y, 3}}));
}

TEST(DatasetTest, setCoord_multiple) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{4});
  const auto time = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{3});
  const auto row = makeVariable<double>(Dims{Dim::Y}, Shape{4});
  const auto group = makeVariable<double>(Dims{});

  Dataset d({{"a", makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 4})}});
  d.setCoord(Dim::X, x);
  d.setCoord(Dim::Time, time);
  d.setCoord(Dim::Y, y);
  d.setCoord(Dim::Row, row);
  d.setCoord(Dim::Group, group);

  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 4}, {Dim::Y, 3}}));
  ASSERT_EQ(d.coords()[Dim::X], x);
  ASSERT_EQ(d.coords()[Dim::Time], time);
  ASSERT_EQ(d.coords()[Dim::Y], y);
  ASSERT_EQ(d.coords()[Dim::Row], row);
  ASSERT_EQ(d.coords()[Dim::Group], group);
}

TEST(DatasetTest, setCoord_dims_mismatch) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{3, 4})}});

  ASSERT_THROW(d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3})),
               except::DimensionError);
  ASSERT_TRUE(d.coords().empty());

  ASSERT_THROW(d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::Z}, Shape{3})),
               except::DimensionError);
  ASSERT_TRUE(d.coords().empty());

  ASSERT_THROW(d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X, Dim::Y},
                                                       Shape{6, 3})),
               except::DimensionError);
  ASSERT_TRUE(d.coords().empty());
}

TEST(DatasetTest, setCoord_invalid_dataset) {
  Dataset d;
  ASSERT_THROW(d.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{3})),
               except::DatasetError);
}

TEST(DatasetTest, setCoords) {
  Dataset a({{"a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2})}});
  Dataset b = a;

  a.setCoord(Dim::X, makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2}));
  a.setCoord(Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{3}));
  b.setCoords(a.coords());
  ASSERT_EQ(a.coords(), b.coords());
}

TEST(DatasetTest, setCoords_too_large) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2})}});

  const Coords coords(
      Dimensions({{Dim::X, 4}, {Dim::Y, 2}}),
      {
          {Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{4})},
          {Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{2})},
      });
  ASSERT_THROW(d.setCoords(coords), except::DimensionError);
  ASSERT_TRUE(d.coords().empty());
}

TEST(DatasetTest, setCoords_too_small) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2})}});

  const Coords coords(
      Dimensions({{Dim::X, 2}, {Dim::Y, 2}}),
      {
          {Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{2})},
          {Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{2})},
      });
  ASSERT_THROW(d.setCoords(coords), except::DimensionError);
  ASSERT_TRUE(d.coords().empty());
}

TEST(DatasetTest, setCoords_too_many_dims) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2})}});

  const Coords coords(
      Dimensions({{Dim::X, 2}, {Dim::Y, 2}, {Dim::Z, 3}}),
      {
          {Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{2})},
          {Dim::Y, makeVariable<double>(Dims{Dim::Y}, Shape{2})},
      });
  ASSERT_THROW(d.setCoords(coords), except::DimensionError);
  ASSERT_TRUE(d.coords().empty());
}

TEST(DatasetTest, setCoords_too_few_dims) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 2})}});

  const Coords coords(
      Dimensions({{Dim::X, 2}}),
      {
          {Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{2})},
      });
  ASSERT_THROW(d.setCoords(coords), except::DimensionError);
  ASSERT_TRUE(d.coords().empty());
}

TEST(DatasetTest, set_item_mask) {
  Dataset d(
      {{"x", makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3})}});
  const auto var =
      makeVariable<bool>(Dims{Dim::X}, Shape{3}, Values{false, true, false});
  d["x"].masks().set("mask", var);
  EXPECT_TRUE(d["x"].masks().contains("mask"));
}

TEST(DatasetTest, setData_with_and_without_variances) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{3});
  Dataset d({}, {{Dim::X, var}});

  ASSERT_NO_THROW(d.setData("a", var));
  ASSERT_EQ(d.size(), 1);

  ASSERT_NO_THROW(d.setData("b", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setData("a", var));
  ASSERT_EQ(d.size(), 2);

  ASSERT_NO_THROW(d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{3},
                                                      Values{1, 1, 1},
                                                      Variances{0, 0, 0})));
  ASSERT_EQ(d.size(), 2);
}

void check_array_shared(Dataset &ds, const std::string &name,
                        const DataArray &array,
                        const bool shared_coord = true) {
  EXPECT_EQ(ds[name], array);
  // Data and meta data are shared
  EXPECT_TRUE(ds[name].data().is_same(array.data()));
  EXPECT_EQ(ds[name].coords()[Dim::X].is_same(array.coords()[Dim::X]),
            shared_coord);
  EXPECT_TRUE(ds[name].masks()["mask"].is_same(array.masks()["mask"]));
  // Metadata *dicts* are not shared
  ds.coords().erase(Dim::X);
  EXPECT_NE(ds[name].coords(), array.coords());
  EXPECT_TRUE(array.coords().contains(Dim::X));
  ds[name].masks().erase("mask");
  EXPECT_NE(ds[name].masks(), array.masks());
  EXPECT_TRUE(array.masks().contains("mask"));
}

TEST(DatasetTest, setData_from_DataArray) {
  const auto array = make_data_array_1d();
  Dataset ds({{"b", makeVariable<double>(array.dims())}});
  ds.setData("a", array);
  check_array_shared(ds, "a", array);
}

TEST(DatasetTest, setData_from_DataArray_replace) {
  const auto array1 = make_data_array_1d(0);
  const auto array2 = make_data_array_1d(1);
  const auto original = copy(array1);
  Dataset ds({{"a", array1}});
  ds.setData("a", array2);
  EXPECT_EQ(array1, original);     // setData does not copy elements
  const bool shared_coord = false; // coord exists in dataset, not replaced
  check_array_shared(ds, "a", array2, shared_coord);
}

TEST(DatasetTest, setData_preserves_rvalue_input) {
  const auto array1 = make_data_array_1d(0);
  Dataset ds({{"a", array1}});
  ds.setData("b", ds["a"]);
  EXPECT_EQ(ds, Dataset({{"a", make_data_array_1d(0)},
                         {"b", make_data_array_1d(0)}}));
}

TEST(DatasetTest, setData_cannot_decrease_ndim) {
  const auto xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{2});

  Dataset d({{"x", xy}});
  ASSERT_THROW(d.setData("x", x), scipp::except::DimensionError);
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 2}, {Dim::Y, 3}}));
}

TEST(DatasetTest, setData_cannot_increase_ndim) {
  const auto xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{2});

  Dataset d({{"x", x}});
  ASSERT_THROW(d.setData("x", xy), scipp::except::DimensionError);
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 2}}));
}

TEST(DatasetTest, setData_cannot_insert_lower_ndim) {
  const auto xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 3});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3});

  Dataset d({{"a", xy}});
  ASSERT_THROW(d.setData("b", x), scipp::except::DimensionError);
}

TEST(DatasetTest, setData_cannot_insert_higher_ndim) {
  const auto xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 3});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3});

  Dataset d({{"a", x}});
  ASSERT_THROW(d.setData("b", xy), scipp::except::DimensionError);
}

TEST(DatasetTest, setData_sets_sizes) {
  const auto xy = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 3});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{4});

  Dataset d({{"a", xy}});
  ASSERT_EQ(d.dims(), Dimensions({{Dim::X, 4}, {Dim::Y, 3}}));
  d.setCoord(Dim::X, x); // can insert coord because setData set the dims.
  ASSERT_EQ(d.coords()[Dim::X], x);
}

TEST(DatasetTest, setData_with_mismatched_dims) {
  scipp::index expected_size = 2;
  const auto original =
      makeVariable<double>(Dims{Dim::X}, Shape{expected_size});
  const auto mismatched =
      makeVariable<double>(Dims{Dim::X}, Shape{expected_size + 1});
  Dataset d({{"a", original}});
  ASSERT_THROW(d.setData("a", mismatched), except::DimensionError);
}

TEST(DatasetTest,
     setData_with_dims_extensions_but_coord_mismatch_does_not_modify) {
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{3, 4});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  DataArray da1(x, {{Dim::X, x}});
  DataArray da2(y, {{Dim::X, y}}); // note Dim::X
  Dataset d({{"da1", da1}});
  ASSERT_THROW(d.setData("da2", da2), except::CoordMismatchError);
  EXPECT_EQ(d.sizes(), da1.dims());
}

TEST(DatasetTest, DataArrayView_setData) {
  const auto var = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  Dataset d({{"a", var}, {"b", var}});

  EXPECT_THROW(d["a"].setData(makeVariable<double>(Dims{Dim::X}, Shape{4})),
               except::DimensionError);
  EXPECT_EQ(d["a"].data(), var);
  EXPECT_NO_THROW(d["a"].setData(var + var));
  EXPECT_EQ(d["a"].data(), var + var);
}

TEST(DatasetTest, setData_invalid_dataset) {
  Dataset d;
  ASSERT_THROW(d.setData("a", makeVariable<double>(Dims{Dim::X}, Shape{3})),
               except::DatasetError);
}

TEST(DatasetTest, setCoord_with_name_matching_data_name) {
  Dataset d({{"a", makeVariable<double>(Dims{Dim::X}, Shape{3})},
             {"b", makeVariable<double>(Dims{Dim::X}, Shape{3})}});

  // It is possible to set labels with a name matching data. However, there is
  // no special meaning attached to this. In particular it is *not* linking the
  // labels to that data item.
  ASSERT_NO_THROW(d.setCoord(Dim("a"), makeVariable<double>(Values{double{}})));
  ASSERT_EQ(d.size(), 2);
  ASSERT_EQ(d.coords().size(), 1);
  ASSERT_EQ(d["a"].coords().size(), 1);
  ASSERT_EQ(d["b"].coords().size(), 1);
}

TEST(DatasetTest, iterators_return_types) {
  Dataset d({{"a", makeVariable<double>()}});
  ASSERT_TRUE((std::is_same_v<decltype(*d.begin()), DataArray>));
  ASSERT_TRUE((std::is_same_v<decltype(*d.end()), DataArray>));
}

TEST(DatasetTest, const_iterators_return_types) {
  const Dataset d({{"a", makeVariable<double>()}});
  ASSERT_TRUE((std::is_same_v<decltype(*d.begin()), DataArray>));
  ASSERT_TRUE((std::is_same_v<decltype(*d.end()), DataArray>));
}

TEST(DatasetTest, iterators) {
  DataArray da1(makeVariable<double>(Dims{Dim::X}, Shape{2}));
  DataArray da2(makeVariable<double>(Dims{Dim::X}, Shape{2}));
  da2.coords().set(Dim::X, makeVariable<double>(Dims{Dim::X}, Shape{2}));
  Dataset d({{"data1", da1}, {"data2", da2}});

  da1.coords().set(Dim::X, da2.coords()[Dim::X]);
  const std::vector expected_arrays{da1, da2};
  const std::vector actual_arrays(d.begin(), d.end());
  EXPECT_EQ(actual_arrays, expected_arrays);

  const std::vector<std::string> names{"data1", "data2"};
  const std::vector actual_names(d.keys_begin(), d.keys_end());
  EXPECT_EQ(actual_names, names);

  const std::vector<std::pair<std::string, DataArray>> expected_items{
      {"data1", da1}, {"data2", da2}};
  const std::vector actual_items(d.items_begin(), d.items_end());
  EXPECT_EQ(actual_items, expected_items);
}

TEST(DatasetTest, slice_temporary) {
  DatasetFactory factory;
  auto dataset = factory.make().slice({Dim::X, 1});
  ASSERT_TRUE((std::is_same_v<decltype(dataset), Dataset>));
}

TEST(DatasetTest, construct_from_slice) {
  DatasetFactory factory;
  const auto dataset = factory.make();
  const auto slice = dataset.slice({Dim::X, 1});
  const Dataset from_slice = copy(slice);
  ASSERT_EQ(from_slice, dataset.slice({Dim::X, 1}));
}

TEST(DatasetTest, construct_dataarray_from_slice) {
  DatasetFactory factory;
  const auto dataset = factory.make("data");
  const auto slice = dataset["data"].slice({Dim::X, 1});
  const DataArray from_slice = copy(slice);
  ASSERT_EQ(from_slice, dataset["data"].slice({Dim::X, 1}));
}

TEST(DatasetTest, slice_validation_simple) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  Dataset dataset({{"a", var}}, {{Dim::X, var}});
  EXPECT_THROW(dataset.slice(Slice{Dim::Y, 0, 1}), except::SliceError);
  EXPECT_THROW(dataset.slice(Slice{Dim::X, 0, 4}), except::SliceError);
  EXPECT_THROW(dataset.slice(Slice{Dim::X, -1, 0}), except::SliceError);
  EXPECT_NO_THROW(dataset.slice(Slice{Dim::X, 0, 1}));
}

TEST(DatasetTest, slice_with_no_coords) {
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  Dataset ds({{"a", var}});
  // No dataset coords. slicing should still work.
  auto slice = ds.slice(Slice{Dim::X, 0, 2});
  auto extents = slice["a"].data().dims()[Dim::X];
  EXPECT_EQ(extents, 2);
}

TEST(DatasetTest, slice_validation_complex) {
  auto var1 = makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  auto var2 = makeVariable<double>(Dims{Dim::Y}, Shape{4}, Values{1, 2, 3, 4});
  Dataset ds({{"a", var1 * var2}}, {{Dim::X, var1}, {Dim::Y, var2}});

  // Slice arguments applied in order.
  EXPECT_NO_THROW(ds.slice(Slice{Dim::X, 0, 3}).slice(Slice{Dim::X, 1, 2}));
  // Reverse order. Invalid slice creation should be caught up front.
  EXPECT_THROW(ds.slice(Slice{Dim::X, 1, 2}).slice(Slice{Dim::X, 0, 3}),
               except::SliceError);
}

TEST(DatasetTest, extract_coord) {
  DatasetFactory factory;
  const auto ref = factory.make();
  Dataset ds = copy(ref);
  auto coord = ds.coords()[Dim::X];
  auto ptr = ds.coords()[Dim::X].values<double>().data();
  auto var = ds.coords().extract(Dim::X);
  EXPECT_EQ(var.values<double>().data(), ptr);
  EXPECT_FALSE(ds.coords().contains(Dim::X));
  ds.setCoord(Dim::X, coord);
  EXPECT_EQ(ref, ds);

  ds.coords().erase(Dim::X);
  EXPECT_FALSE(ds.coords().contains(Dim::X));
  ds.setCoord(Dim::X, coord);
  EXPECT_EQ(ref, ds);
}

TEST(DatasetTest, cannot_set_or_erase_item_coord) {
  DatasetFactory factory;
  auto ds = factory.make("data");
  ASSERT_TRUE(ds.contains("data"));
  ASSERT_THROW(ds["data"].coords().erase(Dim::X), except::DataArrayError);
  ASSERT_TRUE(ds.coords().contains(Dim::X));
  ASSERT_THROW(ds["data"].coords().set(Dim("new"), ds.coords()[Dim::X]),
               except::DataArrayError);
  ASSERT_FALSE(ds.coords().contains(Dim("new")));
}

TEST(DatasetTest, item_coord_cannot_change_coord) {
  DatasetFactory factory;
  Dataset ds = factory.make("data");
  const auto original = copy(ds.coords()[Dim::X]);
  ASSERT_THROW(ds["data"].coords()[Dim::X] += original, except::VariableError);
  ASSERT_EQ(ds.coords()[Dim::X], original);
}

TEST(DatasetTest, set_erase_item_mask) {
  DatasetFactory factory;
  auto ds = factory.make("data");
  const auto mask = makeVariable<double>(Values{true});
  ds["data"].masks().set("item-mask", mask);
  EXPECT_TRUE(ds["data"].masks().contains("item-mask"));
  ds["data"].masks().erase("item-mask");
  EXPECT_FALSE(ds["data"].masks().contains("item-mask"));
}

TEST(DatasetTest, item_name) {
  DatasetFactory factory;
  const auto dataset = factory.make("data");
  const DataArray array(dataset["data"]);
  EXPECT_EQ(array, dataset["data"]);
  // Comparison ignores the name, so this is tested separately.
  EXPECT_EQ(dataset["data"].name(), "data");
  EXPECT_EQ(array.name(), "data");
}

TEST(DatasetTest, self_nesting) {
  const Dataset inner(
      {{"data", makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2})}});
  Variable var = makeVariable<Dataset>(Values{inner});

  const Dataset nested_in_data({{"nested", var}});
  ASSERT_THROW_DISCARD(var.value<Dataset>() = nested_in_data,
                       std::invalid_argument);

  const Dataset nested_in_coord({}, {{Dim::X, var}});
  ASSERT_THROW_DISCARD(var.value<Dataset>() = nested_in_coord,
                       std::invalid_argument);
}

TEST(DatasetTest, drop_coords) {
  const auto array = make_data_array_1d();
  Dataset dset({{"a", array}});
  Dataset expected_dset({{"a", array.drop_coords(std::vector{Dim{"scalar"}})}});
  auto new_dset = dset.drop_coords(std::vector{Dim{"scalar"}});
  ASSERT_EQ(new_dset, expected_dset);
}

struct DatasetRenameTest : public ::testing::Test {
  DatasetRenameTest() {
    DatasetFactory factory(Dimensions({{Dim::X, 3}, {Dim::Y, 4}}));
    factory.seed(98741);
    d = factory.make("data_xy");
    original = d;
  }

protected:
  Dataset d;
  Dataset original;
};

TEST_F(DatasetRenameTest, fail_duplicate_dim) {
  ASSERT_THROW_DISCARD(d.rename_dims({{Dim::X, Dim::Y}}),
                       except::DimensionError);
  ASSERT_EQ(d, original);
}

TEST_F(DatasetRenameTest, fail_duplicate_in_edge_dim_of_coord) {
  auto ds = copy(d);
  const Dim dim{"edge"};
  ds.setCoord(dim, makeVariable<double>(Dims{dim}, Shape{2}));
  ASSERT_THROW_DISCARD(ds.rename_dims({{Dim::X, dim}}), except::DimensionError);
}

TEST_F(DatasetRenameTest, fail_duplicate_in_edge_dim_in_data_array_coord) {
  auto da = copy(d["data_xy"]);
  const Dim dim{"edge"};
  da.coords().set(dim, makeVariable<double>(Dims{dim}, Shape{2}));
  ASSERT_THROW_DISCARD(da.rename_dims({{Dim::X, dim}}), except::DimensionError);
}

TEST_F(DatasetRenameTest, existing) {
  auto out = d.rename_dims({{Dim::X, Dim::X}});
  ASSERT_EQ(d, original);
  ASSERT_EQ(out, original);
}

TEST_F(DatasetRenameTest, back_and_forth) {
  auto tmp = d.rename_dims({{Dim::X, Dim::Row}});
  EXPECT_NE(tmp, original);
  d = tmp.rename_dims({{Dim::Row, Dim::X}});
  EXPECT_EQ(d, original);
}

TEST_F(DatasetRenameTest, rename_dims) {
  // Same as in fixture
  DatasetFactory factory(Dimensions({{Dim::Row, 3}, {Dim::Y, 4}}));
  factory.seed(98741);
  auto expected = factory.make("data_xy");
  // Set coord names to the same as in `d`.
  expected.coords().set(Dim{"x"}, expected.coords().extract(Dim{"row"}));
  expected.coords().set(Dim{"labels_x"},
                        expected.coords().extract(Dim{"labels_row"}));
  expected.coords().set(Dim{"xy"}, expected.coords().extract(Dim{"rowy"}));

  auto renamed = d.rename_dims({{Dim::X, Dim::Row}});
  EXPECT_EQ(renamed, expected);
}

TEST_F(DatasetRenameTest, no_data) {
  // Same as in fixture
  DatasetFactory factory(Dimensions({{Dim::Row, 3}, {Dim::Y, 4}}));
  factory.seed(98741);
  auto expected = factory.make("data_xy");
  // Set coord names to the same as in `d`.
  expected.coords().set(Dim{"x"}, expected.coords().extract(Dim{"row"}));
  expected.coords().set(Dim{"labels_x"},
                        expected.coords().extract(Dim{"labels_row"}));
  expected.coords().set(Dim{"xy"}, expected.coords().extract(Dim{"rowy"}));

  d.erase("data_xy");
  expected.erase("data_xy");

  auto renamed = d.rename_dims({{Dim::X, Dim::Row}});
  EXPECT_TRUE(renamed.is_valid());
  EXPECT_EQ(renamed, expected);
}
