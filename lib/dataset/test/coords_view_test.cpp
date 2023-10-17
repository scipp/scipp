// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/core/dimensions.h"
#include "scipp/dataset/bins.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/variable/shape.h"
#include "test_macros.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::dataset;

template <typename T> class CoordsViewTest : public ::testing::Test {
protected:
  template <class D>
  std::conditional_t<std::is_same_v<T, Coords>, Dataset, const Dataset> &
  access(D &dataset) {
    return dataset;
  }
};

using CoordsViewTypes = ::testing::Types<Coords, const Coords>;
TYPED_TEST_SUITE(CoordsViewTest, CoordsViewTypes);

TYPED_TEST(CoordsViewTest, empty) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();
  ASSERT_TRUE(coords.empty());
  ASSERT_EQ(coords.size(), 0);
}

TYPED_TEST(CoordsViewTest, bad_item_access) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();
  ASSERT_ANY_THROW(coords[Dim::X]);
}

TYPED_TEST(CoordsViewTest, item_access) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{4, 5});
  Dataset d({{"a", x}}, {{Dim::X, x}, {Dim::Y, y}});

  const auto coords = TestFixture::access(d).coords();
  ASSERT_EQ(coords[Dim::X], x);
  ASSERT_EQ(coords[Dim::Y], y);
}

TYPED_TEST(CoordsViewTest, iterators_empty_coords) {
  Dataset d;
  const auto coords = TestFixture::access(d).coords();

  ASSERT_NO_THROW(coords.begin());
  ASSERT_NO_THROW(coords.end());
  EXPECT_EQ(coords.begin(), coords.end());
}

TYPED_TEST(CoordsViewTest, iterators) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{4, 5});
  Dataset d({{"a", x}}, {{Dim::X, x}, {Dim::Y, y}});
  const auto coords = TestFixture::access(d).coords();

  ASSERT_NO_THROW(coords.begin());
  ASSERT_NO_THROW(coords.end());

  auto it = coords.begin();
  ASSERT_NE(it, coords.end());
  if (it->first == Dim::X)
    EXPECT_EQ(it->second, x);
  else
    EXPECT_EQ(it->second, y);

  ASSERT_NO_THROW(++it);
  ASSERT_NE(it, coords.end());
  if (it->first == Dim::Y)
    EXPECT_EQ(it->second, y);
  else
    EXPECT_EQ(it->second, x);

  ASSERT_NO_THROW(++it);
  ASSERT_EQ(it, coords.end());
}

TYPED_TEST(CoordsViewTest, find_and_contains) {
  DatasetFactory factory({{Dim::X, 3}, {Dim::Y, 2}});
  auto dataset = factory.make();
  const auto coords = TestFixture::access(dataset).coords();

  EXPECT_EQ(coords.find(Dim::Row), coords.end());
  EXPECT_EQ(coords.find(Dim::X)->first, Dim::X);
  EXPECT_EQ(coords.find(Dim::X)->second, coords[Dim::X]);
  EXPECT_FALSE(coords.contains(Dim::Row));
  EXPECT_TRUE(coords.contains(Dim::X));

  EXPECT_EQ(coords.find(Dim::Y)->first, Dim::Y);
  EXPECT_EQ(coords.find(Dim::Y)->second, coords[Dim::Y]);
}

TEST(MutableCoordsViewTest, item_write) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{4, 5});
  const auto x_reference =
      makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1.5, 2.0, 3.0});
  const auto y_reference =
      makeVariable<double>(Dims{Dim::Y}, Shape{2}, Values{4.5, 5.0});
  Dataset d({{"a", x}}, {{Dim::X, x}, {Dim::Y, y}});

  auto &coords = d.coords();
  coords[Dim::X].values<double>()[0] += 0.5;
  coords[Dim::Y].values<double>()[0] += 0.5;
  ASSERT_EQ(coords[Dim::X], x_reference);
  ASSERT_EQ(coords[Dim::Y], y_reference);
}

TEST(SizedDictTest, default_init) {
  const Coords coords;
  ASSERT_TRUE(coords.empty());
  ASSERT_EQ(coords.sizes(), Sizes());
}

TEST(SizedDictTest, init_with_dict) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 5});
  const Coords::holder_type dict({{Dim::X, x}, {Dim::Y, y}});
  const Coords coords(Dimensions({{Dim::X, 3}, {Dim::Y, 5}}), dict);
  ASSERT_EQ(coords.size(), 2);
  ASSERT_EQ(coords.sizes(), Dimensions({{Dim::X, 3}, {Dim::Y, 5}}));
  ASSERT_EQ(coords[Dim::X], x);
  ASSERT_EQ(coords[Dim::Y], y);
}

TEST(SizedDictTest, init_with_dict_bad_sizes) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 5});
  const Coords::holder_type dict({{Dim::X, x}, {Dim::Y, y}});
  ASSERT_THROW(Coords(Dimensions({{Dim::X, 4}, {Dim::Y, 5}}), dict),
               except::DimensionError);
  ASSERT_THROW(Coords(Dimensions({{Dim::X, 3}, {Dim::Y, 4}}), dict),
               except::DimensionError);
  ASSERT_THROW(Coords(Dimensions({{Dim::X, 3}, {Dim::Y, 7}}), dict),
               except::DimensionError);
}

TEST(SizedDictTest, init_with_dict_decreasing_sizes) {
  const auto a = makeVariable<int>(Dims{Dim::X}, Shape{5});
  const auto b = makeVariable<int>(Dims{Dim::X}, Shape{4});
  const auto c = makeVariable<int>(Dims{Dim::X}, Shape{3});
  const Coords::holder_type dict({{Dim{"a"}, a}, {Dim{"b"}, b}, {Dim{"c"}, c}});
  ASSERT_THROW(Coords(AutoSizeTag{}, dict), except::DimensionError);
}

TEST(SizedDictTest, init_with_dict_increasing_sizes) {
  const auto a = makeVariable<int>(Dims{Dim::X}, Shape{3});
  const auto b = makeVariable<int>(Dims{Dim::X}, Shape{4});
  const auto c = makeVariable<int>(Dims{Dim::X}, Shape{5});
  const Coords::holder_type dict({{Dim{"a"}, a}, {Dim{"b"}, b}, {Dim{"c"}, c}});
  ASSERT_THROW(Coords(AutoSizeTag{}, dict), except::DimensionError);
}

TEST(SizedDictTest, copy) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  const auto y = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{4, 5});
  const Coords::holder_type dict({{Dim::X, x}, {Dim::Y, y}});
  const Coords coords(Dimensions({{Dim::X, 3}, {Dim::Y, 5}}), dict);

  const Coords copy = coords;

  ASSERT_EQ(copy.size(), 2);
  ASSERT_EQ(copy.sizes(), Dimensions({{Dim::X, 3}, {Dim::Y, 5}}));
  ASSERT_EQ(copy[Dim::X], x);
  ASSERT_EQ(copy[Dim::Y], y);
}

TEST(SizedDictTest, copy_resets_readonly_flag) {
  const Coords a;
  const Coords copy_a = a;
  ASSERT_FALSE(copy_a.is_readonly());

  Coords b;
  b.set_readonly();
  const Coords copy_b = b;
  ASSERT_FALSE(copy_b.is_readonly());
}

TEST(SizedDictTest, set_scalar_without_setting_sizes) {
  const auto scalar = makeVariable<double>(Dims{}, Values{1.2});
  Coords coords;
  coords.set(Dim::X, scalar);
  ASSERT_EQ(coords.size(), 1);
  ASSERT_EQ(coords.sizes(), Sizes());
  ASSERT_EQ(coords[Dim::X], scalar);
}

TEST(SizedDictTest, set_length_2_without_setting_sizes) {
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{4, 2});
  Coords coords;
  coords.set(Dim::X, x);
  ASSERT_EQ(coords.size(), 1);
  ASSERT_EQ(coords.sizes(), Sizes());
  ASSERT_EQ(coords[Dim::X], x);
}

TEST(SizedDictTest, set_sizes_explicitly) {
  Coords coords;
  coords.setSizes(Dimensions({{Dim::X, 3}, {Dim::Y, 5}}));
  ASSERT_EQ(coords.sizes(), Dimensions({{Dim::X, 3}, {Dim::Y, 5}}));

  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3});
  coords.set(Dim::X, x);
  ASSERT_EQ(coords[Dim::X], x);
}

TEST(SizedDictTest, set_bin_edges) {
  const auto x2y3 = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 3});
  const auto x2y4 = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{2, 4});
  const auto x3y3 = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 3});
  const auto x3y4 = makeVariable<double>(Dims{Dim::X, Dim::Y}, Shape{3, 4});
  // Single bin in extra dim, not dim of data
  const auto x2_extra = makeVariable<double>(Dims{Dim::X, Dim::Z}, Shape{2, 2});
  const auto x3_extra = makeVariable<double>(Dims{Dim::X, Dim::Z}, Shape{3, 2});
  DataArray da(x2y3);
  ASSERT_NO_THROW(da.coords().set(Dim::X, x2y4));
  ASSERT_NO_THROW(da.coords().set(Dim::X, transpose(x2y4)));
  ASSERT_NO_THROW(da.coords().set(Dim::X, x3y3));
  ASSERT_NO_THROW(da.coords().set(Dim::X, transpose(x3y3)));
  ASSERT_NO_THROW(da.coords().set(Dim::X, x2_extra));
  ASSERT_NO_THROW(da.coords().set(Dim::X, transpose(x2_extra)));
  ASSERT_THROW(da.coords().set(Dim::X, x3y4), except::DimensionError);
  ASSERT_THROW(da.coords().set(Dim::X, transpose(x3y4)),
               except::DimensionError);
  ASSERT_THROW(da.coords().set(Dim::X, x3_extra), except::DimensionError);
  ASSERT_THROW(da.coords().set(Dim::X, transpose(x3_extra)),
               except::DimensionError);
}

TEST(SizedDictTest, rename_dims) {
  const auto a = makeVariable<int>(Dims{Dim{"a"}}, Shape{2}, Values{1, 2});
  const auto b = makeVariable<int>(Dims{Dim{"b"}}, Shape{2}, Values{3, 4});
  const auto c = makeVariable<int>(Dims{Dim{"c"}}, Shape{2}, Values{5, 6});
  const DataArray da(a + b + c, {{Dim("a"), a}, {Dim("b"), b}, {Dim("c"), c}});
  const auto &coords = da.coords();
  const auto renamed = coords.rename_dims({{Dim("b"), Dim("r")}});
  const auto r = b.rename_dims({{Dim("b"), Dim("r")}});
  const Coords expected(renamed.sizes(),
                        {{Dim("a"), a}, {Dim("b"), r}, {Dim("c"), c}});
  for (auto it1 = renamed.begin(), it2 = expected.begin(); it1 != renamed.end();
       ++it1, ++it2) {
    ASSERT_NE(it2, expected.end());
    EXPECT_EQ(*it1, *it2);
  }
}

TEST(SizedDictTest, insertion_preserves_alignement) {
  const auto a = makeVariable<int>(Dims{Dim{"a"}}, Shape{2}, Values{1, 2});
  auto b = makeVariable<int>(Dims{Dim{"b"}}, Shape{2}, Values{3, 4});
  auto c = makeVariable<int>(Dims{Dim{"c"}}, Shape{2}, Values{5, 6});
  const auto d = makeVariable<int>(Dims{Dim{"d"}}, Shape{2}, Values{7, 8});
  b.set_aligned(false);
  c.set_aligned(false);

  DataArray da(a + b + c + d, {{Dim("a"), a}, {Dim("b"), b}});
  auto &coords = da.coords();

  EXPECT_TRUE(coords.at(Dim("a")).is_aligned());
  EXPECT_FALSE(coords.at(Dim("b")).is_aligned());

  coords.set(Dim("c"), c);
  coords.set(Dim("d"), d);
  EXPECT_TRUE(coords.at(Dim("a")).is_aligned());
  EXPECT_FALSE(coords.at(Dim("b")).is_aligned());
  EXPECT_FALSE(coords.at(Dim("c")).is_aligned());
  EXPECT_TRUE(coords.at(Dim("d")).is_aligned());
}

TEST(SizedDictTest, set_alignment) {
  const auto a = makeVariable<int>(Dims{Dim{"a"}}, Shape{2}, Values{1, 2});
  const auto b = makeVariable<int>(Dims{Dim{"b"}}, Shape{2}, Values{3, 4});
  const auto c = makeVariable<int>(Dims{Dim{"c"}}, Shape{2}, Values{5, 6});
  DataArray da(a + b + c, {{Dim("a"), a}, {Dim("b"), b}, {Dim("c"), c}});
  auto &coords = da.coords();

  EXPECT_TRUE(coords.at(Dim("a")).is_aligned());
  EXPECT_TRUE(coords.at(Dim("b")).is_aligned());
  EXPECT_TRUE(coords.at(Dim("c")).is_aligned());

  // This has no effect because coords.at returns a new variable.
  coords.at(Dim("b")).set_aligned(false);
  EXPECT_TRUE(coords.at(Dim("b")).is_aligned());

  coords.set_aligned(Dim("a"), false);
  coords.set_aligned(Dim("c"), false);
  EXPECT_FALSE(coords.at(Dim("a")).is_aligned());
  EXPECT_TRUE(coords.at(Dim("b")).is_aligned());
  EXPECT_FALSE(coords.at(Dim("c")).is_aligned());

  coords.set_aligned(Dim("c"), true);
  EXPECT_FALSE(coords.at(Dim("a")).is_aligned());
  EXPECT_TRUE(coords.at(Dim("b")).is_aligned());
  EXPECT_TRUE(coords.at(Dim("c")).is_aligned());
}

TEST(SizedDictTest, set_alignement_requires_writable) {
  const auto a = makeVariable<int>(Dims{Dim{"a"}}, Shape{2}, Values{1, 2});
  DataArray da(a, {{Dim("a"), a}});
  auto &coords = da.coords();
  coords.set_readonly();

  EXPECT_THROW(coords.set_aligned(Dim("a"), false), except::DataArrayError);
}

TEST(SizedDictTest, copy_preserves_alignment) {
  const auto a = makeVariable<int>(Dims{Dim{"a"}}, Shape{2}, Values{1, 2});
  auto b = makeVariable<int>(Dims{Dim{"b"}}, Shape{2}, Values{3, 4});
  b.set_aligned(false);

  DataArray da(a + b, {{Dim("a"), a}, {Dim("b"), b}});
  auto copied = da.coords();
  EXPECT_TRUE(copied.at(Dim("a")).is_aligned());
  EXPECT_FALSE(copied.at(Dim("b")).is_aligned());
}

TEST(SizedDictTest, data_array_copy_preserves_alignment) {
  const auto a = makeVariable<int>(Dims{Dim{"a"}}, Shape{2}, Values{1, 2});
  auto b = makeVariable<int>(Dims{Dim{"b"}}, Shape{2}, Values{3, 4});
  b.set_aligned(false);

  const DataArray da(a + b, {{Dim("a"), a}, {Dim("b"), b}});
  const auto shallow_copied(da);
  auto &shallow_coords = shallow_copied.coords();
  EXPECT_TRUE(shallow_coords.at(Dim("a")).is_aligned());
  EXPECT_FALSE(shallow_coords.at(Dim("b")).is_aligned());

  const auto deep_copied = copy(da);
  auto &deep_coords = deep_copied.coords();
  EXPECT_TRUE(deep_coords.at(Dim("a")).is_aligned());
  EXPECT_FALSE(deep_coords.at(Dim("b")).is_aligned());
}

TEST(SizedDictTest, binned_data_array_copy_preserves_alignment) {
  const auto a = makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{1, 2});
  auto b = makeVariable<int>(Dims{Dim::X}, Shape{2}, Values{3, 4});
  b.set_aligned(false);

  const DataArray da(a + b, {{Dim("a"), a}, {Dim("b"), b}});
  const auto indices = makeVariable<scipp::index_pair>(
      Dims{Dim::X}, Shape{2}, Values{std::pair{0, 1}, std::pair{1, 2}});
  const auto binned = make_bins(indices, Dim::X, da);

  const auto shallow_copied(binned);
  const auto shallow_buffer =
      std::get<2>(shallow_copied.constituents<DataArray>());
  EXPECT_TRUE(shallow_buffer.coords().at(Dim("a")).is_aligned());
  EXPECT_FALSE(shallow_buffer.coords().at(Dim("b")).is_aligned());

  const auto deep_copied = copy(binned);
  const auto deep_buffer = std::get<2>(deep_copied.constituents<DataArray>());
  EXPECT_TRUE(deep_buffer.coords().at(Dim("a")).is_aligned());
  EXPECT_FALSE(deep_buffer.coords().at(Dim("b")).is_aligned());
}

TEST(SizedDictTest, merge_from) {
  const Dimensions dims({{Dim::X, 3}, {Dim::Y, 4}});
  const auto x = makeVariable<double>(Dims{Dim::X}, Shape{3});
  const auto y = makeVariable<double>(Dims{Dim::Y}, Shape{4});
  const auto xy = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{4, 3});
  const Coords a(dims, {{Dim::X, x}});
  const Coords b(dims, {{Dim::Y, y}, {Dim::Row, xy}});

  const auto merged = a.merge_from(b);
  EXPECT_EQ(merged.sizes(), dims);
  EXPECT_EQ(merged.size(), 3);
  EXPECT_EQ(merged[Dim::X], x);
  EXPECT_EQ(merged[Dim::Y], y);
  EXPECT_EQ(merged[Dim::Row], xy);
}
