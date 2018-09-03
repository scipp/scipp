/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "dimensions.h"
#include "tags.h"
#include "variable.h"

TEST(Variable, construct) {
  ASSERT_NO_THROW(makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), 2));
  const auto a = makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), 2);
  const auto &data = a.get<Data::Value>();
  EXPECT_EQ(data.size(), 2);
}

TEST(Variable, construct_fail) {
  ASSERT_ANY_THROW(makeVariable<Data::Value>(Dimensions(), 2));
  ASSERT_ANY_THROW(makeVariable<Data::Value>(Dimensions(Dimension::Tof, 1), 2));
  ASSERT_ANY_THROW(makeVariable<Data::Value>(Dimensions(Dimension::Tof, 3), 2));
}

TEST(Variable, span_references_Variable) {
  auto a = makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), 2);
  auto observer = a.get<const Data::Value>();
  // This line does not compile, const-correctness works:
  // observer[0] = 1.0;

  // Note: This also has the "usual" problem of copy-on-write: This non-const
  // call can invalidate the references stored in observer if the underlying
  // data was shared.
  auto span = a.get<Data::Value>();

  EXPECT_EQ(span.size(), 2);
  span[0] = 1.0;
  EXPECT_EQ(observer[0], 1.0);
}

TEST(Variable, sharing) {
  const auto a1 = makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), 2);
  const auto a2(a1);
  // TODO Should we require the use of `const` with the tag if Variable is
  // const?
  EXPECT_EQ(&a1.get<Data::Value>()[0], &a2.get<Data::Value>()[0]);
}

TEST(Variable, copy) {
  const auto a1 =
      makeVariable<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  const auto &data1 = a1.get<Data::Value>();
  EXPECT_EQ(data1[0], 1.1);
  EXPECT_EQ(data1[1], 2.2);
  auto a2(a1);
  EXPECT_EQ(&a1.get<Data::Value>()[0], &a2.get<const Data::Value>()[0]);
  EXPECT_NE(&a1.get<Data::Value>()[0], &a2.get<Data::Value>()[0]);
  const auto &data2 = a2.get<Data::Value>();
  EXPECT_EQ(data2[0], 1.1);
  EXPECT_EQ(data2[1], 2.2);
}

TEST(Variable, ragged) {
  const auto raggedSize = makeVariable<Data::DimensionSize>(
      Dimensions(Dimension::Spectrum, 2), {2l, 3l});
  EXPECT_EQ(raggedSize.dimensions().volume(), 2);
  Dimensions dimensions;
  dimensions.add(Dimension::Tof, raggedSize);
  dimensions.add(Dimension::Spectrum, 2);
  EXPECT_EQ(dimensions.volume(), 5);
  ASSERT_NO_THROW(makeVariable<Data::Value>(dimensions, 5));
  ASSERT_ANY_THROW(makeVariable<Data::Value>(dimensions, 4));
}

TEST(Variable, operator_equals) {
  const auto a = makeVariable<Data::Value>({Dimension::Tof, 2}, {1.1, 2.2});
  const auto a_copy(a);
  const auto b = makeVariable<Data::Value>({Dimension::Tof, 2}, {1.1, 2.2});
  const auto diff1 = makeVariable<Data::Value>({Dimension::Tof, 2}, {1.1, 2.1});
  const auto diff2 = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});
  auto diff3(a);
  diff3.setName("test");
  auto diff4(a);
  diff4.setUnit(Unit::Id::Length);
  EXPECT_EQ(a, a);
  EXPECT_EQ(a, a_copy);
  EXPECT_EQ(a, b);
  EXPECT_EQ(b, a);
  EXPECT_FALSE(a == diff1);
  EXPECT_FALSE(a == diff2);
  EXPECT_FALSE(a == diff3);
  EXPECT_FALSE(a == diff4);
}

TEST(Variable, operator_plus_equal) {
  auto a = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});

  EXPECT_NO_THROW(a += a);
  EXPECT_EQ(a.get<Data::Value>()[0], 2.2);
  EXPECT_EQ(a.get<Data::Value>()[1], 4.4);

  auto different_name(a);
  different_name.setName("test");
  EXPECT_NO_THROW(a += different_name);
}

TEST(Variable, operator_plus_equal_automatic_broadcast_of_rhs) {
  auto a = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});

  auto fewer_dimensions = makeVariable<Data::Value>({}, {1.0});

  EXPECT_NO_THROW(a += fewer_dimensions);
  EXPECT_EQ(a.get<Data::Value>()[0], 2.1);
  EXPECT_EQ(a.get<Data::Value>()[1], 3.2);
}

TEST(Variable, operator_plus_equal_transpose) {
  auto a = makeVariable<Data::Value>(
      Dimensions({{Dimension::X, 2}, {Dimension::Y, 3}}),
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  auto transpose = makeVariable<Data::Value>(
      Dimensions({{Dimension::Y, 3}, {Dimension::X, 2}}),
      {1.0, 3.0, 5.0, 2.0, 4.0, 6.0});

  EXPECT_NO_THROW(a += transpose);
  EXPECT_EQ(a.get<Data::Value>()[0], 2.0);
  EXPECT_EQ(a.get<Data::Value>()[1], 4.0);
  EXPECT_EQ(a.get<Data::Value>()[2], 6.0);
  EXPECT_EQ(a.get<Data::Value>()[3], 8.0);
  EXPECT_EQ(a.get<Data::Value>()[4], 10.0);
  EXPECT_EQ(a.get<Data::Value>()[5], 12.0);
}

TEST(Variable, operator_plus_equal_different_dimensions) {
  auto a = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});

  auto different_dimensions =
      makeVariable<Data::Value>({Dimension::Y, 2}, {1.1, 2.2});
  EXPECT_THROW_MSG(a += different_dimensions, std::runtime_error,
                   "Cannot add Variables: Dimensions do not match.");
}

TEST(Variable, operator_plus_equal_different_unit) {
  auto a = makeVariable<Data::Value>({Dimension::X, 2}, {1.1, 2.2});

  auto different_unit(a);
  different_unit.setUnit(Unit::Id::Length);
  EXPECT_THROW_MSG(a += different_unit, std::runtime_error,
                   "Cannot add Variables: Units do not match.");
}

TEST(Variable, operator_plus_equal_non_arithmetic_type) {
  auto a = makeVariable<Data::String>({Dimension::X, 1}, {std::string("test")});
  EXPECT_THROW_MSG(a += a, std::runtime_error,
                   "Cannot add strings. Use append() instead.");
}

TEST(Variable, operator_plus_equal_different_variables_different_element_type) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto b = makeVariable<Data::Int>({Dimension::X, 1}, {2l});
  EXPECT_THROW_MSG(a += b, std::runtime_error,
                   "Cannot apply arithmetic operation to Variables: Underlying "
                   "data types do not match.");
}

TEST(Variable, operator_plus_equal_different_variables_same_element_type) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto b = makeVariable<Data::Variance>({Dimension::X, 1}, {2.0});
  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.get<Data::Value>()[0], 3.0);
}

TEST(Variable, operator_times_equal) {
  auto a = makeVariable<Coord::X>({Dimension::X, 2}, {2.0, 3.0});

  EXPECT_EQ(a.unit(), Unit::Id::Length);
  EXPECT_NO_THROW(a *= a);
  EXPECT_EQ(a.get<Data::Value>()[0], 4.0);
  EXPECT_EQ(a.get<Data::Value>()[1], 9.0);
  EXPECT_EQ(a.unit(), Unit::Id::Area);
}

TEST(Variable, setSlice) {
  Dimensions dims(Dimension::Tof, 1);
  const auto parent = makeVariable<Data::Value>(
      Dimensions({{Dimension::X, 4}, {Dimension::Y, 2}, {Dimension::Z, 3}}),
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0,
       14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});
  const auto empty = makeVariable<Data::Value>(
      Dimensions({{Dimension::X, 4}, {Dimension::Y, 2}, {Dimension::Z, 3}}),
      24);

  auto d(empty);
  EXPECT_NE(parent, d);
  for (const gsl::index index : {0, 1, 2, 3})
    d.setSlice(slice(parent, Dimension::X, index), Dimension::X, index);
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const gsl::index index : {0, 1})
    d.setSlice(slice(parent, Dimension::Y, index), Dimension::Y, index);
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const gsl::index index : {0, 1, 2})
    d.setSlice(slice(parent, Dimension::Z, index), Dimension::Z, index);
  EXPECT_EQ(parent, d);
}

TEST(Variable, slice) {
  Dimensions dims(Dimension::Tof, 1);
  const auto parent = makeVariable<Data::Value>(
      Dimensions({{Dimension::X, 4}, {Dimension::Y, 2}, {Dimension::Z, 3}}),
      {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0,
       14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const gsl::index index : {0, 1, 2, 3}) {
    auto sliceX = slice(parent, Dimension::X, index);
    ASSERT_EQ(sliceX.dimensions(),
              Dimensions({{Dimension::Y, 2}, {Dimension::Z, 3}}));
    EXPECT_EQ(sliceX.get<const Data::Value>()[0], index + 1.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[1], index + 5.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[2], index + 9.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[3], index + 13.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[4], index + 17.0);
    EXPECT_EQ(sliceX.get<const Data::Value>()[5], index + 21.0);
  }

  for (const gsl::index index : {0, 1}) {
    auto sliceY = slice(parent, Dimension::Y, index);
    ASSERT_EQ(sliceY.dimensions(),
              Dimensions({{Dimension::X, 4}, {Dimension::Z, 3}}));
    const auto &data = sliceY.get<const Data::Value>();
    for (const gsl::index z : {0, 1, 2}) {
      EXPECT_EQ(data[4 * z + 0], 4 * index + 8 * z + 1.0);
      EXPECT_EQ(data[4 * z + 1], 4 * index + 8 * z + 2.0);
      EXPECT_EQ(data[4 * z + 2], 4 * index + 8 * z + 3.0);
      EXPECT_EQ(data[4 * z + 3], 4 * index + 8 * z + 4.0);
    }
  }

  for (const gsl::index index : {0, 1, 2}) {
    auto sliceZ = slice(parent, Dimension::Z, index);
    ASSERT_EQ(sliceZ.dimensions(),
              Dimensions({{Dimension::X, 4}, {Dimension::Y, 2}}));
    const auto &data = sliceZ.get<const Data::Value>();
    for (gsl::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }
}

TEST(Variable, concatenate) {
  Dimensions dims(Dimension::Tof, 1);
  auto a = makeVariable<Data::Value>(dims, {1.0});
  auto b = makeVariable<Data::Value>(dims, {2.0});
  a.setUnit(Unit::Id::Length);
  b.setUnit(Unit::Id::Length);
  auto ab = concatenate(Dimension::Tof, a, b);
  ASSERT_EQ(ab.size(), 2);
  EXPECT_EQ(ab.unit(), Unit(Unit::Id::Length));
  const auto &data = ab.get<Data::Value>();
  EXPECT_EQ(data[0], 1.0);
  EXPECT_EQ(data[1], 2.0);
  auto ba = concatenate(Dimension::Tof, b, a);
  const auto abba = concatenate(Dimension::Q, ab, ba);
  ASSERT_EQ(abba.size(), 4);
  EXPECT_EQ(abba.dimensions().count(), 2);
  const auto &data2 = abba.get<Data::Value>();
  EXPECT_EQ(data2[0], 1.0);
  EXPECT_EQ(data2[1], 2.0);
  EXPECT_EQ(data2[2], 2.0);
  EXPECT_EQ(data2[3], 1.0);
  const auto ababbaba = concatenate(Dimension::Tof, abba, abba);
  ASSERT_EQ(ababbaba.size(), 8);
  const auto &data3 = ababbaba.get<Data::Value>();
  EXPECT_EQ(data3[0], 1.0);
  EXPECT_EQ(data3[1], 2.0);
  EXPECT_EQ(data3[2], 1.0);
  EXPECT_EQ(data3[3], 2.0);
  EXPECT_EQ(data3[4], 2.0);
  EXPECT_EQ(data3[5], 1.0);
  EXPECT_EQ(data3[6], 2.0);
  EXPECT_EQ(data3[7], 1.0);
  const auto abbaabba = concatenate(Dimension::Q, abba, abba);
  ASSERT_EQ(abbaabba.size(), 8);
  const auto &data4 = abbaabba.get<Data::Value>();
  EXPECT_EQ(data4[0], 1.0);
  EXPECT_EQ(data4[1], 2.0);
  EXPECT_EQ(data4[2], 2.0);
  EXPECT_EQ(data4[3], 1.0);
  EXPECT_EQ(data4[4], 1.0);
  EXPECT_EQ(data4[5], 2.0);
  EXPECT_EQ(data4[6], 2.0);
  EXPECT_EQ(data4[7], 1.0);
}

TEST(Variable, concatenate_volume_with_slice) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto aa = concatenate(Dimension::X, a, a);
  EXPECT_NO_THROW(concatenate(Dimension::X, aa, a));
}

TEST(Variable, concatenate_slice_with_volume) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto aa = concatenate(Dimension::X, a, a);
  EXPECT_NO_THROW(concatenate(Dimension::X, a, aa));
}

TEST(Variable, concatenate_fail) {
  Dimensions dims(Dimension::Tof, 1);
  auto a = makeVariable<Data::Value>(dims, {1.0});
  auto b = makeVariable<Data::Value>(dims, {2.0});
  auto c = makeVariable<Data::Variance>(dims, {2.0});
  a.setName("data");
  EXPECT_THROW_MSG(concatenate(Dimension::Tof, a, b), std::runtime_error,
                   "Cannot concatenate Variables: Names do not match.");
  c.setName("data");
  EXPECT_THROW_MSG(concatenate(Dimension::Tof, a, c), std::runtime_error,
                   "Cannot concatenate Variables: Data types do not match.");
  auto aa = concatenate(Dimension::Tof, a, a);
  EXPECT_THROW_MSG(
      concatenate(Dimension::Q, a, aa), std::runtime_error,
      "Cannot concatenate Variables: Dimension extents do not match.");
}

TEST(Variable, concatenate_unit_fail) {
  Dimensions dims(Dimension::X, 1);
  auto a = makeVariable<Data::Value>(dims, {1.0});
  auto b(a);
  EXPECT_NO_THROW(concatenate(Dimension::X, a, b));
  a.setUnit(Unit::Id::Length);
  EXPECT_THROW_MSG(concatenate(Dimension::X, a, b), std::runtime_error,
                   "Cannot concatenate Variables: Units do not match.");
  b.setUnit(Unit::Id::Length);
  EXPECT_NO_THROW(concatenate(Dimension::X, a, b));
}
