#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "data_array.h"
#include "dimensions.h"
#include "variable.h"

TEST(DataArray, construct) {
  ASSERT_NO_THROW(makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 2), 2));
  const auto a = makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 2), 2);
  const auto &data = a.get<Data::Value>();
  EXPECT_EQ(data.size(), 2);
}

TEST(DataArray, construct_fail) {
  ASSERT_ANY_THROW(makeDataArray<Data::Value>(Dimensions(), 2));
  ASSERT_ANY_THROW(
      makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 1), 2));
  ASSERT_ANY_THROW(
      makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 3), 2));
}

TEST(DataArray, sharing) {
  const auto a1 = makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 2), 2);
  const auto a2(a1);
  EXPECT_EQ(&a1.get<Data::Value>(), &a2.get<Data::Value>());
}

TEST(DataArray, copy) {
  const auto a1 =
      makeDataArray<Data::Value>(Dimensions(Dimension::Tof, 2), {1.1, 2.2});
  const auto &data1 = a1.get<Data::Value>();
  EXPECT_EQ(data1[0], 1.1);
  EXPECT_EQ(data1[1], 2.2);
  auto a2(a1);
  EXPECT_EQ(&a1.get<Data::Value>(), &a2.get<const Data::Value>());
  EXPECT_NE(&a1.get<Data::Value>(), &a2.get<Data::Value>());
  const auto &data2 = a2.get<Data::Value>();
  EXPECT_EQ(data2[0], 1.1);
  EXPECT_EQ(data2[1], 2.2);
}

TEST(DataArray, ragged) {
  const auto raggedSize = makeDataArray<Data::DimensionSize>(
      Dimensions(Dimension::SpectrumNumber, 2), {2l, 3l});
  EXPECT_EQ(raggedSize.dimensions().volume(), 2);
  Dimensions dimensions;
  dimensions.add(Dimension::Tof, raggedSize);
  dimensions.add(Dimension::SpectrumNumber, 2);
  EXPECT_EQ(dimensions.volume(), 5);
  ASSERT_NO_THROW(makeDataArray<Data::Value>(dimensions, 5));
  ASSERT_ANY_THROW(makeDataArray<Data::Value>(dimensions, 4));
}

TEST(DataArray, concatenate) {
  Dimensions dims(Dimension::Tof, 1);
  auto a = makeDataArray<Data::Value>(dims, {1.0});
  auto b = makeDataArray<Data::Value>(dims, {2.0});
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

TEST(DataArray, concatenate_fail) {
  Dimensions dims(Dimension::Tof, 1);
  auto a = makeDataArray<Data::Value>(dims, {1.0});
  auto b = makeDataArray<Data::Value>(dims, {2.0});
  auto c = makeDataArray<Data::Error>(dims, {2.0});
  a.setName("data");
  EXPECT_THROW_MSG(concatenate(Dimension::Tof, a, b), std::runtime_error,
                   "Cannot concatenate DataArrays: Names do not match.");
  c.setName("data");
  EXPECT_THROW_MSG(concatenate(Dimension::Tof, a, c), std::runtime_error,
                   "Cannot concatenate DataArrays: Data types do not match.");
  auto aa = concatenate(Dimension::Tof, a, a);
  // TODO better size check, the following should work:
  // EXPECT_NO_THROW(concatenate(Dimension::Tof, a, aa));
  EXPECT_THROW_MSG(concatenate(Dimension::Q, a, aa), std::runtime_error,
                   "Cannot concatenate DataArrays: Dimensions do not match.");
}

TEST(DataArray, concatenate_unit_fail) {
  Dimensions dims(Dimension::X, 1);
  auto a = makeDataArray<Data::Value>(dims, {1.0});
  auto b(a);
  EXPECT_NO_THROW(concatenate(Dimension::X, a, b));
  a.setUnit(Unit::Id::Length);
  EXPECT_THROW_MSG(concatenate(Dimension::X, a, b), std::runtime_error,
                   "Cannot concatenate DataArrays: Units do not match.");
  b.setUnit(Unit::Id::Length);
  EXPECT_NO_THROW(concatenate(Dimension::X, a, b));
}
