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
      Dimensions(Dimension::SpectrumNumber, 2), {2l, 3l});
  EXPECT_EQ(raggedSize.dimensions().volume(), 2);
  Dimensions dimensions;
  dimensions.add(Dimension::Tof, raggedSize);
  dimensions.add(Dimension::SpectrumNumber, 2);
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

  auto different_dimensions =
      makeVariable<Data::Value>({Dimension::Y, 2}, {1.1, 2.2});
  EXPECT_THROW_MSG(a += different_dimensions, std::runtime_error,
                   "Cannot add Variables: Dimensions do not match.");

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
                   "Cannot add Variables: Underlying data types do not match.");
}

TEST(Variable, operator_plus_equal_different_variables_same_element_type) {
  auto a = makeVariable<Data::Value>({Dimension::X, 1}, {1.0});
  auto b = makeVariable<Data::Error>({Dimension::X, 1}, {2.0});
  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.get<Data::Value>()[0], 3.0);
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

TEST(Variable, concatenate_fail) {
  Dimensions dims(Dimension::Tof, 1);
  auto a = makeVariable<Data::Value>(dims, {1.0});
  auto b = makeVariable<Data::Value>(dims, {2.0});
  auto c = makeVariable<Data::Error>(dims, {2.0});
  a.setName("data");
  EXPECT_THROW_MSG(concatenate(Dimension::Tof, a, b), std::runtime_error,
                   "Cannot concatenate Variables: Names do not match.");
  c.setName("data");
  EXPECT_THROW_MSG(concatenate(Dimension::Tof, a, c), std::runtime_error,
                   "Cannot concatenate Variables: Data types do not match.");
  auto aa = concatenate(Dimension::Tof, a, a);
  // TODO better size check, the following should work:
  // EXPECT_NO_THROW(concatenate(Dimension::Tof, a, aa));
  EXPECT_THROW_MSG(concatenate(Dimension::Q, a, aa), std::runtime_error,
                   "Cannot concatenate Variables: Dimensions do not match.");
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
