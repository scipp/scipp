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

using namespace scipp;
using namespace scipp::core;

TEST(Variable, construct) {
  ASSERT_NO_THROW(Variable(Data::Value, Dimensions(Dim::Tof, 2)));
  ASSERT_NO_THROW(Variable(Data::Value, Dimensions(Dim::Tof, 2), 2));
  const Variable a(Data::Value, Dimensions(Dim::Tof, 2));
  const auto &data = a.get(Data::Value);
  EXPECT_EQ(data.size(), 2);
}

TEST(Variable, construct_fail) {
  ASSERT_ANY_THROW(Variable(Data::Value, Dimensions(), 2));
  ASSERT_ANY_THROW(Variable(Data::Value, Dimensions(Dim::Tof, 1), 2));
  ASSERT_ANY_THROW(Variable(Data::Value, Dimensions(Dim::Tof, 3), 2));
}

TEST(Variable, makeVariable_custom_type) {
  auto doubles = makeVariable<double>(Data::Value, {});
  auto floats = makeVariable<float>(Data::Value, {});

  ASSERT_NO_THROW(doubles.get(Data::Value));
  // Data::Value defaults to double, so this throws.
  ASSERT_ANY_THROW(floats.get(Data::Value));

  ASSERT_NO_THROW(doubles.span<double>());
  ASSERT_NO_THROW(floats.span<float>());

  ASSERT_ANY_THROW(doubles.span<float>());
  ASSERT_ANY_THROW(floats.span<double>());

  ASSERT_TRUE((std::is_same<decltype(doubles.span<double>())::element_type,
                            double>::value));
  ASSERT_TRUE((std::is_same<decltype(floats.span<float>())::element_type,
                            float>::value));
}

TEST(Variable, makeVariable_custom_type_initializer_list) {
  Variable doubles(Data::Value, {Dim::X, 2}, {1, 2});
  auto ints = makeVariable<int32_t>(Data::Value, {Dim::X, 2}, {1.1, 2.2});

  // Passed ints but uses default type based on tag.
  ASSERT_NO_THROW(doubles.span<double>());
  // Passed doubles but explicit type overrides.
  ASSERT_NO_THROW(ints.span<int32_t>());
}

TEST(Variable, dtype) {
  auto doubles = makeVariable<double>(Data::Value, {});
  auto floats = makeVariable<float>(Data::Value, {});
  EXPECT_EQ(doubles.dtype(), dtype<double>);
  EXPECT_NE(doubles.dtype(), dtype<float>);
  EXPECT_NE(floats.dtype(), dtype<double>);
  EXPECT_EQ(floats.dtype(), dtype<float>);
  EXPECT_EQ(doubles.dtype(), doubles.dtype());
  EXPECT_EQ(floats.dtype(), floats.dtype());
  EXPECT_NE(doubles.dtype(), floats.dtype());
}

TEST(Variable, span_references_Variable) {
  Variable a(Data::Value, Dimensions(Dim::Tof, 2));
  auto observer = a.get(Data::Value);
  // This line does not compile, const-correctness works:
  // observer[0] = 1.0;

  // Note: This also has the "usual" problem of copy-on-write: This non-const
  // call can invalidate the references stored in observer if the underlying
  // data was shared.
  auto span = a.get(Data::Value);

  EXPECT_EQ(span.size(), 2);
  span[0] = 1.0;
  EXPECT_EQ(observer[0], 1.0);
}

TEST(Variable, copy) {
  const Variable a1(Data::Value, {Dim::Tof, 2}, {1.1, 2.2});
  const auto &data1 = a1.get(Data::Value);
  EXPECT_EQ(data1[0], 1.1);
  EXPECT_EQ(data1[1], 2.2);
  auto a2(a1);
  EXPECT_NE(&a1.get(Data::Value)[0], &a2.get(Data::Value)[0]);
  EXPECT_NE(&a1.get(Data::Value)[0], &a2.get(Data::Value)[0]);
  const auto &data2 = a2.get(Data::Value);
  EXPECT_EQ(data2[0], 1.1);
  EXPECT_EQ(data2[1], 2.2);
}

TEST(Variable, operator_equals) {
  const Variable a(Data::Value, {Dim::Tof, 2}, {1.1, 2.2});
  const auto a_copy(a);
  const Variable b(Data::Value, {Dim::Tof, 2}, {1.1, 2.2});
  const Variable diff1(Data::Value, {Dim::Tof, 2}, {1.1, 2.1});
  const Variable diff2(Data::Value, {Dim::X, 2}, {1.1, 2.2});
  auto diff3(a);
  diff3.setName("test");
  auto diff4(a);
  diff4.setUnit(units::m);
  EXPECT_EQ(a, a);
  EXPECT_EQ(a, a_copy);
  EXPECT_EQ(a, b);
  EXPECT_EQ(b, a);
  EXPECT_FALSE(a == diff1);
  EXPECT_FALSE(a == diff2);
  EXPECT_FALSE(a == diff3);
  EXPECT_FALSE(a == diff4);
}

TEST(Variable, operator_equals_mismatching_dtype) {
  auto a = makeVariable<double>(Data::Value, {});
  auto b = makeVariable<float>(Data::Value, {});
  EXPECT_NE(a, b);
}

TEST(Variable, operator_unary_minus) {
  const Variable a(Data::Value, {Dim::X, 2}, {1.1, 2.2});
  auto b = -a;
  EXPECT_EQ(a.get(Data::Value)[0], 1.1);
  EXPECT_EQ(a.get(Data::Value)[1], 2.2);
  EXPECT_EQ(b.get(Data::Value)[0], -1.1);
  EXPECT_EQ(b.get(Data::Value)[1], -2.2);
}

TEST(VariableSlice, unary_minus) {
  const Variable a(Data::Value, {Dim::X, 2}, {1.1, 2.2});
  auto b = -a(Dim::X, 1);
  EXPECT_EQ(a.get(Data::Value)[0], 1.1);
  EXPECT_EQ(a.get(Data::Value)[1], 2.2);
  EXPECT_EQ(b.get(Data::Value)[0], -2.2);
}

TEST(Variable, operator_plus_equal) {
  Variable a(Data::Value, {Dim::X, 2}, {1.1, 2.2});

  EXPECT_NO_THROW(a += a);
  EXPECT_EQ(a.get(Data::Value)[0], 2.2);
  EXPECT_EQ(a.get(Data::Value)[1], 4.4);

  auto different_name(a);
  different_name.setName("test");
  EXPECT_NO_THROW(a += different_name);
}

TEST(Variable, operator_plus_equal_automatic_broadcast_of_rhs) {
  Variable a(Data::Value, {Dim::X, 2}, {1.1, 2.2});

  Variable fewer_dimensions(Data::Value, {}, {1.0});

  EXPECT_NO_THROW(a += fewer_dimensions);
  EXPECT_EQ(a.get(Data::Value)[0], 2.1);
  EXPECT_EQ(a.get(Data::Value)[1], 3.2);
}

TEST(Variable, operator_plus_equal_transpose) {
  Variable a(Data::Value, Dimensions({{Dim::Y, 3}, {Dim::X, 2}}),
             {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  Variable transpose(Data::Value, Dimensions({{Dim::X, 2}, {Dim::Y, 3}}),
                     {1.0, 3.0, 5.0, 2.0, 4.0, 6.0});

  EXPECT_NO_THROW(a += transpose);
  EXPECT_EQ(a.get(Data::Value)[0], 2.0);
  EXPECT_EQ(a.get(Data::Value)[1], 4.0);
  EXPECT_EQ(a.get(Data::Value)[2], 6.0);
  EXPECT_EQ(a.get(Data::Value)[3], 8.0);
  EXPECT_EQ(a.get(Data::Value)[4], 10.0);
  EXPECT_EQ(a.get(Data::Value)[5], 12.0);
}

TEST(Variable, operator_plus_equal_different_dimensions) {
  Variable a(Data::Value, {Dim::X, 2}, {1.1, 2.2});

  Variable different_dimensions(Data::Value, {Dim::Y, 2}, {1.1, 2.2});
  EXPECT_THROW_MSG(a += different_dimensions, std::runtime_error,
                   "Expected {{Dim::X, 2}} to contain {{Dim::Y, 2}}.");
}

TEST(Variable, operator_plus_equal_different_unit) {
  Variable a(Data::Value, {Dim::X, 2}, {1.1, 2.2});

  auto different_unit(a);
  different_unit.setUnit(units::m);
  EXPECT_THROW_MSG(a += different_unit, except::UnitMismatchError,
                   "Expected dimensionless to be equal to m.");
}

TEST(Variable, operator_plus_equal_non_arithmetic_type) {
  auto a = makeVariable<std::string>(Data::Value, {Dim::X, 1},
                                     {std::string("test")});
  EXPECT_THROW_MSG(a += a, std::runtime_error,
                   "Cannot apply operation, requires addable type.");
}

TEST(Variable, operator_plus_equal_different_variables_different_element_type) {
  Variable a(Data::Value, {Dim::X, 1}, {1.0});
  auto b = makeVariable<int64_t>(Data::Value, {Dim::X, 1}, {2});
  EXPECT_THROW_MSG(a += b, except::TypeError,
                   "Expected item dtype double, got int64.");
}

TEST(Variable, operator_plus_equal_different_variables_same_element_type) {
  Variable a(Data::Value, {Dim::X, 1}, {1.0});
  Variable b(Data::Variance, {Dim::X, 1}, {2.0});
  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.get(Data::Value)[0], 3.0);
}

TEST(Variable, operator_plus_equal_scalar) {
  Variable a(Data::Value, {Dim::X, 2}, {1.1, 2.2});

  EXPECT_NO_THROW(a += 1.0);
  EXPECT_EQ(a.get(Data::Value)[0], 2.1);
  EXPECT_EQ(a.get(Data::Value)[1], 3.2);
}

TEST(Variable, operator_plus_equal_custom_type) {
  auto a = makeVariable<float>(Data::Value, {Dim::X, 2}, {1.1f, 2.2f});

  EXPECT_NO_THROW(a += a);
  EXPECT_EQ(a.span<float>()[0], 2.2f);
  EXPECT_EQ(a.span<float>()[1], 4.4f);

  auto different_name(a);
  different_name.setName("test");
  EXPECT_NO_THROW(a += different_name);
}

TEST(Variable, operator_times_equal) {
  Variable a(Coord::X, {Dim::X, 2}, {2.0, 3.0});

  EXPECT_EQ(a.unit(), units::m);
  EXPECT_NO_THROW(a *= a);
  EXPECT_EQ(a.get(Coord::X)[0], 4.0);
  EXPECT_EQ(a.get(Coord::X)[1], 9.0);
  EXPECT_EQ(a.unit(), units::m * units::m);
}

TEST(Variable, operator_times_equal_scalar) {
  Variable a(Coord::X, {Dim::X, 2}, {2.0, 3.0});

  EXPECT_EQ(a.unit(), units::m);
  EXPECT_NO_THROW(a *= 2.0);
  EXPECT_EQ(a.get(Coord::X)[0], 4.0);
  EXPECT_EQ(a.get(Coord::X)[1], 6.0);
  EXPECT_EQ(a.unit(), units::m);
}

TEST(Variable, operator_times_can_broadcast) {
  Variable a(Data::Value, {Dim::X, 2}, {0.5, 1.5});
  Variable b(Data::Value, {Dim::Y, 2}, {2.0, 3.0});

  auto ab = a * b;
  Variable reference(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}},
                     {1.0, 3.0, 1.5, 4.5});
  EXPECT_EQ(ab, reference);
}

TEST(Variable, operator_divide_equal) {
  Variable a(Data::Value, {Dim::X, 2}, {2.0, 3.0});
  Variable b(Data::Value, {}, {2.0});
  b.setUnit(units::m);

  EXPECT_NO_THROW(a /= b);
  EXPECT_EQ(a.get(Data::Value)[0], 1.0);
  EXPECT_EQ(a.get(Data::Value)[1], 1.5);
  EXPECT_EQ(a.unit(), units::dimensionless / units::m);
}

TEST(Variable, operator_divide_equal_self) {
  Variable a(Coord::X, {Dim::X, 2}, {2.0, 3.0});

  EXPECT_EQ(a.unit(), units::m);
  EXPECT_NO_THROW(a /= a);
  EXPECT_EQ(a.get(Coord::X)[0], 1.0);
  EXPECT_EQ(a.get(Coord::X)[1], 1.0);
  EXPECT_EQ(a.unit(), units::dimensionless);
}

TEST(Variable, operator_divide_equal_scalar) {
  Variable a(Coord::X, {Dim::X, 2}, {2.0, 4.0});

  EXPECT_EQ(a.unit(), units::m);
  EXPECT_NO_THROW(a /= 2.0);
  EXPECT_EQ(a.get(Coord::X)[0], 1.0);
  EXPECT_EQ(a.get(Coord::X)[1], 2.0);
  EXPECT_EQ(a.unit(), units::m);
}

TEST(Variable, operator_divide_scalar_double) {
  const auto a = makeVariable<double>(Coord::X, {Dim::X, 2}, {2.0, 4.0});
  const auto result = 1.111 / a;
  EXPECT_EQ(result.span<double>()[0], 1.111 / 2.0);
  EXPECT_EQ(result.span<double>()[1], 1.111 / 4.0);
  EXPECT_EQ(result.unit(), units::dimensionless / units::m);
}

TEST(Variable, operator_divide_scalar_float) {
  const auto a = makeVariable<float>(Coord::X, {Dim::X, 2}, {2.0, 4.0});
  const auto result = 1.111 / a;
  EXPECT_EQ(result.span<float>()[0], 1.111f / 2.0f);
  EXPECT_EQ(result.span<float>()[1], 1.111f / 4.0f);
  EXPECT_EQ(result.unit(), units::dimensionless / units::m);
}

TEST(Variable, setSlice) {
  Dimensions dims(Dim::Tof, 1);
  const Variable parent(
      Data::Value, Dimensions({{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 3}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});
  const Variable empty(Data::Value,
                       Dimensions({{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 3}}), 24);

  auto d(empty);
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1, 2, 3})
    d(Dim::X, index).assign(parent(Dim::X, index));
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1})
    d(Dim::Y, index).assign(parent(Dim::Y, index));
  EXPECT_EQ(parent, d);

  d = empty;
  EXPECT_NE(parent, d);
  for (const scipp::index index : {0, 1, 2})
    d(Dim::Z, index).assign(parent(Dim::Z, index));
  EXPECT_EQ(parent, d);
}

TEST(Variable, slice) {
  Dimensions dims(Dim::Tof, 1);
  const Variable parent(
      Data::Value, Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 4}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const scipp::index index : {0, 1, 2, 3}) {
    Variable sliceX = parent(Dim::X, index);
    ASSERT_EQ(sliceX.dimensions(), Dimensions({{Dim::Z, 3}, {Dim::Y, 2}}));
    auto base = static_cast<double>(index);
    EXPECT_EQ(sliceX.get(Data::Value)[0], base + 1.0);
    EXPECT_EQ(sliceX.get(Data::Value)[1], base + 5.0);
    EXPECT_EQ(sliceX.get(Data::Value)[2], base + 9.0);
    EXPECT_EQ(sliceX.get(Data::Value)[3], base + 13.0);
    EXPECT_EQ(sliceX.get(Data::Value)[4], base + 17.0);
    EXPECT_EQ(sliceX.get(Data::Value)[5], base + 21.0);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceY = parent(Dim::Y, index);
    ASSERT_EQ(sliceY.dimensions(), Dimensions({{Dim::Z, 3}, {Dim::X, 4}}));
    const auto &data = sliceY.get(Data::Value);
    auto base = static_cast<double>(index);
    for (const scipp::index z : {0, 1, 2}) {
      EXPECT_EQ(data[4 * z + 0], 4 * base + 8 * static_cast<double>(z) + 1.0);
      EXPECT_EQ(data[4 * z + 1], 4 * base + 8 * static_cast<double>(z) + 2.0);
      EXPECT_EQ(data[4 * z + 2], 4 * base + 8 * static_cast<double>(z) + 3.0);
      EXPECT_EQ(data[4 * z + 3], 4 * base + 8 * static_cast<double>(z) + 4.0);
    }
  }

  for (const scipp::index index : {0, 1, 2}) {
    Variable sliceZ = parent(Dim::Z, index);
    ASSERT_EQ(sliceZ.dimensions(), Dimensions({{Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.get(Data::Value);
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }
}

TEST(Variable, slice_range) {
  Dimensions dims(Dim::Tof, 1);
  const Variable parent(
      Data::Value, Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 4}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const scipp::index index : {0, 1, 2, 3}) {
    Variable sliceX = parent(Dim::X, index, index + 1);
    ASSERT_EQ(sliceX.dimensions(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 1}}));
    EXPECT_EQ(sliceX.get(Data::Value)[0], index + 1.0);
    EXPECT_EQ(sliceX.get(Data::Value)[1], index + 5.0);
    EXPECT_EQ(sliceX.get(Data::Value)[2], index + 9.0);
    EXPECT_EQ(sliceX.get(Data::Value)[3], index + 13.0);
    EXPECT_EQ(sliceX.get(Data::Value)[4], index + 17.0);
    EXPECT_EQ(sliceX.get(Data::Value)[5], index + 21.0);
  }

  for (const scipp::index index : {0, 1, 2}) {
    Variable sliceX = parent(Dim::X, index, index + 2);
    ASSERT_EQ(sliceX.dimensions(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 2}}));
    EXPECT_EQ(sliceX.get(Data::Value)[0], index + 1.0);
    EXPECT_EQ(sliceX.get(Data::Value)[1], index + 2.0);
    EXPECT_EQ(sliceX.get(Data::Value)[2], index + 5.0);
    EXPECT_EQ(sliceX.get(Data::Value)[3], index + 6.0);
    EXPECT_EQ(sliceX.get(Data::Value)[4], index + 9.0);
    EXPECT_EQ(sliceX.get(Data::Value)[5], index + 10.0);
    EXPECT_EQ(sliceX.get(Data::Value)[6], index + 13.0);
    EXPECT_EQ(sliceX.get(Data::Value)[7], index + 14.0);
    EXPECT_EQ(sliceX.get(Data::Value)[8], index + 17.0);
    EXPECT_EQ(sliceX.get(Data::Value)[9], index + 18.0);
    EXPECT_EQ(sliceX.get(Data::Value)[10], index + 21.0);
    EXPECT_EQ(sliceX.get(Data::Value)[11], index + 22.0);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceY = parent(Dim::Y, index, index + 1);
    ASSERT_EQ(sliceY.dimensions(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 1}, {Dim::X, 4}}));
    const auto &data = sliceY.get(Data::Value);
    for (const scipp::index z : {0, 1, 2}) {
      EXPECT_EQ(data[4 * z + 0], 4 * index + 8 * z + 1.0);
      EXPECT_EQ(data[4 * z + 1], 4 * index + 8 * z + 2.0);
      EXPECT_EQ(data[4 * z + 2], 4 * index + 8 * z + 3.0);
      EXPECT_EQ(data[4 * z + 3], 4 * index + 8 * z + 4.0);
    }
  }

  for (const scipp::index index : {0}) {
    Variable sliceY = parent(Dim::Y, index, index + 2);
    EXPECT_EQ(sliceY, parent);
  }

  for (const scipp::index index : {0, 1, 2}) {
    Variable sliceZ = parent(Dim::Z, index, index + 1);
    ASSERT_EQ(sliceZ.dimensions(),
              Dimensions({{Dim::Z, 1}, {Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.get(Data::Value);
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceZ = parent(Dim::Z, index, index + 2);
    ASSERT_EQ(sliceZ.dimensions(),
              Dimensions({{Dim::Z, 2}, {Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.get(Data::Value);
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[8 + xy], 1.0 + 8 + xy + 8 * index);
  }
}

TEST(Variable, concatenate) {
  Dimensions dims(Dim::Tof, 1);
  Variable a(Data::Value, dims, {1.0});
  Variable b(Data::Value, dims, {2.0});
  a.setUnit(units::m);
  b.setUnit(units::m);
  auto ab = concatenate(a, b, Dim::Tof);
  ASSERT_EQ(ab.size(), 2);
  EXPECT_EQ(ab.unit(), units::Unit(units::m));
  const auto &data = ab.get(Data::Value);
  EXPECT_EQ(data[0], 1.0);
  EXPECT_EQ(data[1], 2.0);
  auto ba = concatenate(b, a, Dim::Tof);
  const auto abba = concatenate(ab, ba, Dim::Q);
  ASSERT_EQ(abba.size(), 4);
  EXPECT_EQ(abba.dimensions().count(), 2);
  const auto &data2 = abba.get(Data::Value);
  EXPECT_EQ(data2[0], 1.0);
  EXPECT_EQ(data2[1], 2.0);
  EXPECT_EQ(data2[2], 2.0);
  EXPECT_EQ(data2[3], 1.0);
  const auto ababbaba = concatenate(abba, abba, Dim::Tof);
  ASSERT_EQ(ababbaba.size(), 8);
  const auto &data3 = ababbaba.get(Data::Value);
  EXPECT_EQ(data3[0], 1.0);
  EXPECT_EQ(data3[1], 2.0);
  EXPECT_EQ(data3[2], 1.0);
  EXPECT_EQ(data3[3], 2.0);
  EXPECT_EQ(data3[4], 2.0);
  EXPECT_EQ(data3[5], 1.0);
  EXPECT_EQ(data3[6], 2.0);
  EXPECT_EQ(data3[7], 1.0);
  const auto abbaabba = concatenate(abba, abba, Dim::Q);
  ASSERT_EQ(abbaabba.size(), 8);
  const auto &data4 = abbaabba.get(Data::Value);
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
  Variable a(Data::Value, {Dim::X, 1}, {1.0});
  auto aa = concatenate(a, a, Dim::X);
  EXPECT_NO_THROW(concatenate(aa, a, Dim::X));
}

TEST(Variable, concatenate_slice_with_volume) {
  Variable a(Data::Value, {Dim::X, 1}, {1.0});
  auto aa = concatenate(a, a, Dim::X);
  EXPECT_NO_THROW(concatenate(a, aa, Dim::X));
}

TEST(Variable, concatenate_fail) {
  Dimensions dims(Dim::Tof, 1);
  Variable a(Data::Value, dims, {1.0});
  Variable b(Data::Value, dims, {2.0});
  Variable c(Data::Variance, dims, {2.0});
  a.setName("data");
  EXPECT_THROW_MSG(concatenate(a, b, Dim::Tof), std::runtime_error,
                   "Cannot concatenate Variables: Names do not match.");
  c.setName("data");
  EXPECT_THROW_MSG(concatenate(a, c, Dim::Tof), std::runtime_error,
                   "Cannot concatenate Variables: Data types do not match.");
  auto aa = concatenate(a, a, Dim::Tof);
  EXPECT_THROW_MSG(
      concatenate(a, aa, Dim::Q), std::runtime_error,
      "Cannot concatenate Variables: Dimension extents do not match.");
}

TEST(Variable, concatenate_unit_fail) {
  Dimensions dims(Dim::X, 1);
  Variable a(Data::Value, dims, {1.0});
  auto b(a);
  EXPECT_NO_THROW(concatenate(a, b, Dim::X));
  a.setUnit(units::m);
  EXPECT_THROW_MSG(concatenate(a, b, Dim::X), std::runtime_error,
                   "Cannot concatenate Variables: Units do not match.");
  b.setUnit(units::m);
  EXPECT_NO_THROW(concatenate(a, b, Dim::X));
}

TEST(Variable, rebin) {
  Variable var(Data::Value, {Dim::X, 2}, {1.0, 2.0});
  var.setUnit(units::counts);
  const Variable oldEdge(Coord::X, {Dim::X, 3}, {1.0, 2.0, 3.0});
  const Variable newEdge(Coord::X, {Dim::X, 2}, {1.0, 3.0});
  auto rebinned = rebin(var, oldEdge, newEdge);
  ASSERT_EQ(rebinned.dimensions().count(), 1);
  ASSERT_EQ(rebinned.dimensions().volume(), 1);
  ASSERT_EQ(rebinned.get(Data::Value).size(), 1);
  EXPECT_EQ(rebinned.get(Data::Value)[0], 3.0);
}

TEST(Variable, sum) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto sumX = sum(var, Dim::X);
  ASSERT_EQ(sumX.dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_TRUE(equals(sumX.get(Data::Value), {3.0, 7.0}));
  auto sumY = sum(var, Dim::Y);
  ASSERT_EQ(sumY.dimensions(), (Dimensions{Dim::X, 2}));
  EXPECT_TRUE(equals(sumY.get(Data::Value), {4.0, 6.0}));
}

TEST(Variable, mean) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto meanX = mean(var, Dim::X);
  ASSERT_EQ(meanX.dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_TRUE(equals(meanX.get(Data::Value), {1.5, 3.5}));
  auto meanY = mean(var, Dim::Y);
  ASSERT_EQ(meanY.dimensions(), (Dimensions{Dim::X, 2}));
  EXPECT_TRUE(equals(meanY.get(Data::Value), {2.0, 3.0}));
}

TEST(Variable, norm_of_scalar) {
  Variable reference(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, -2.0, -3.0, 4.0});
  EXPECT_EQ(norm(var), reference);
}

TEST(Variable, norm_of_vector) {
  Variable reference(Data::Value, {Dim::X, 3}, {sqrt(2), sqrt(2), 2});
  auto var = makeVariable<Eigen::Vector3d>(Data::Value, {Dim::X, 3},
                                           {Eigen::Vector3d{1, 0, -1},
                                            Eigen::Vector3d{1, 1, 0},
                                            Eigen::Vector3d{0, 0, -2}});
  EXPECT_EQ(norm(var), reference);
}

TEST(Variable, sqrt_double) {
  // TODO Currently comparisons of variables do not provide special handling of
  // NaN, so sqrt of negative values will lead variables that are never equal.
  auto reference = makeVariable<double>(Data::Value, {Dim::X, 2}, {1, 2});
  reference.setUnit(units::m);
  auto var = makeVariable<double>(Data::Value, {Dim::X, 2}, {1, 4});
  var.setUnit(units::m * units::m);
  EXPECT_EQ(sqrt(var), reference);
}

TEST(Variable, sqrt_float) {
  auto reference = makeVariable<float>(Data::Value, {Dim::X, 2}, {1, 2});
  reference.setUnit(units::m);
  auto var = makeVariable<float>(Data::Value, {Dim::X, 2}, {1, 4});
  var.setUnit(units::m * units::m);
  EXPECT_EQ(sqrt(var), reference);
}

TEST(Variable, broadcast) {
  Variable reference(Data::Value, {{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 2}},
                     {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4});
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});

  // No change if dimensions exist.
  EXPECT_EQ(broadcast(var, {Dim::X, 2}), var);
  EXPECT_EQ(broadcast(var, {Dim::Y, 2}), var);
  EXPECT_EQ(broadcast(var, {{Dim::Y, 2}, {Dim::X, 2}}), var);

  // No transpose done, should this fail? Failing is not really necessary since
  // we have labeled dimensions.
  EXPECT_EQ(broadcast(var, {{Dim::X, 2}, {Dim::Y, 2}}), var);

  EXPECT_EQ(broadcast(var, {Dim::Z, 3}), reference);
}

TEST(Variable, broadcast_fail) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  EXPECT_THROW_MSG(broadcast(var, {Dim::X, 3}), except::DimensionLengthError,
                   "Expected dimension to be in {{Dim::Y, 2}, {Dim::X, 2}}, "
                   "got Dim::X with mismatching length 3.");
}

TEST(VariableSlice, full_const_view) {
  const Variable var(Coord::X, {{Dim::X, 3}});
  ConstVariableSlice view(var);
  EXPECT_EQ(var.get(Coord::X).data(), view.get(Coord::X).data());
}

TEST(VariableSlice, full_mutable_view) {
  Variable var(Coord::X, {{Dim::X, 3}});
  VariableSlice view(var);
  EXPECT_EQ(var.get(Coord::X).data(), view.get(Coord::X).data());
  EXPECT_EQ(var.get(Coord::X).data(), view.get(Coord::X).data());
}

TEST(VariableSlice, strides) {
  Variable var(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
  EXPECT_EQ(var(Dim::X, 0).strides(), (std::vector<scipp::index>{3}));
  EXPECT_EQ(var(Dim::X, 1).strides(), (std::vector<scipp::index>{3}));
  EXPECT_EQ(var(Dim::Y, 0).strides(), (std::vector<scipp::index>{1}));
  EXPECT_EQ(var(Dim::Y, 1).strides(), (std::vector<scipp::index>{1}));
  EXPECT_EQ(var(Dim::X, 0, 1).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 1, 2).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 0, 1).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 1, 2).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 0, 2).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::X, 1, 3).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 0, 2).strides(), (std::vector<scipp::index>{3, 1}));
  EXPECT_EQ(var(Dim::Y, 1, 3).strides(), (std::vector<scipp::index>{3, 1}));

  EXPECT_EQ(var(Dim::X, 0, 1)(Dim::Y, 0, 1).strides(),
            (std::vector<scipp::index>{3, 1}));

  Variable var3D(Data::Value, {{Dim::Z, 4}, {Dim::Y, 3}, {Dim::X, 2}});
  EXPECT_EQ(var3D(Dim::X, 0, 1)(Dim::Z, 0, 1).strides(),
            (std::vector<scipp::index>{6, 2, 1}));
}

TEST(VariableSlice, get) {
  const Variable var(Data::Value, {Dim::X, 3}, {1, 2, 3});
  EXPECT_EQ(var(Dim::X, 1, 2).get(Data::Value)[0], 2.0);
}

TEST(VariableSlice, slicing_does_not_transpose) {
  Variable var(Data::Value, {{Dim::X, 3}, {Dim::Y, 3}});
  Dimensions expected{{Dim::X, 1}, {Dim::Y, 1}};
  EXPECT_EQ(var(Dim::X, 1, 2)(Dim::Y, 1, 2).dimensions(), expected);
  EXPECT_EQ(var(Dim::Y, 1, 2)(Dim::X, 1, 2).dimensions(), expected);
}

TEST(VariableSlice, minus_equals_failures) {
  Variable var(Data::Value, {{Dim::X, 2}, {Dim::Y, 2}}, {1.0, 2.0, 3.0, 4.0});

  EXPECT_THROW_MSG(var -= var(Dim::X, 0, 1), std::runtime_error,
                   "Expected {{Dim::X, 2}, {Dim::Y, 2}} to contain {{Dim::X, "
                   "1}, {Dim::Y, 2}}.");
}

TEST(VariableSlice, self_overlapping_view_operation) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});

  var -= var(Dim::Y, 0);
  const auto data = var.get(Data::Value);
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  // This is the critical part: After subtracting for y=0 the view points to
  // data containing 0.0, so subsequently the subtraction would have no effect
  // if self-overlap was not taken into account by the implementation.
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
}

TEST(VariableSlice, minus_equals_slice_const_outer) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  const auto copy(var);

  var -= copy(Dim::Y, 0);
  const auto data = var.get(Data::Value);
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
  var -= copy(Dim::Y, 1);
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -4.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableSlice, minus_equals_slice_outer) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::Y, 0);
  const auto data = var.get(Data::Value);
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
  var -= copy(Dim::Y, 1);
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -4.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableSlice, minus_equals_slice_inner) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::X, 0);
  const auto data = var.get(Data::Value);
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 1.0);
  EXPECT_EQ(data[2], 0.0);
  EXPECT_EQ(data[3], 1.0);
  var -= copy(Dim::X, 1);
  EXPECT_EQ(data[0], -2.0);
  EXPECT_EQ(data[1], -1.0);
  EXPECT_EQ(data[2], -4.0);
  EXPECT_EQ(data[3], -3.0);
}

TEST(VariableSlice, minus_equals_slice_of_slice) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::X, 1)(Dim::Y, 1);
  const auto data = var.get(Data::Value);
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -2.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], 0.0);
}

TEST(VariableSlice, minus_equals_nontrivial_slices) {
  Variable source(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}},
                  {11.0, 12.0, 13.0, 21.0, 22.0, 23.0, 31.0, 32.0, 33.0});
  {
    Variable target(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 0, 2)(Dim::Y, 0, 2);
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], -21.0);
    EXPECT_EQ(data[3], -22.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 1, 3)(Dim::Y, 0, 2);
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], -12.0);
    EXPECT_EQ(data[1], -13.0);
    EXPECT_EQ(data[2], -22.0);
    EXPECT_EQ(data[3], -23.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 0, 2)(Dim::Y, 1, 3);
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], -21.0);
    EXPECT_EQ(data[1], -22.0);
    EXPECT_EQ(data[2], -31.0);
    EXPECT_EQ(data[3], -32.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 1, 3)(Dim::Y, 1, 3);
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], -22.0);
    EXPECT_EQ(data[1], -23.0);
    EXPECT_EQ(data[2], -32.0);
    EXPECT_EQ(data[3], -33.0);
  }
}

TEST(VariableSlice, slice_inner_minus_equals) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});

  var(Dim::X, 0) -= var(Dim::X, 1);
  const auto data = var.get(Data::Value);
  EXPECT_EQ(data[0], -1.0);
  EXPECT_EQ(data[1], 2.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableSlice, slice_outer_minus_equals) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});

  var(Dim::Y, 0) -= var(Dim::Y, 1);
  const auto data = var.get(Data::Value);
  EXPECT_EQ(data[0], -2.0);
  EXPECT_EQ(data[1], -2.0);
  EXPECT_EQ(data[2], 3.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableSlice, nontrivial_slice_minus_equals) {
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}},
                    {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2) -= source;
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -21.0);
    EXPECT_EQ(data[4], -22.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}},
                    {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2) -= source;
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], -11.0);
    EXPECT_EQ(data[2], -12.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -21.0);
    EXPECT_EQ(data[5], -22.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}},
                    {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3) -= source;
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -11.0);
    EXPECT_EQ(data[4], -12.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], -21.0);
    EXPECT_EQ(data[7], -22.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}},
                    {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3) -= source;
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -11.0);
    EXPECT_EQ(data[5], -12.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], -21.0);
    EXPECT_EQ(data[8], -22.0);
  }
}

TEST(VariableSlice, nontrivial_slice_minus_equals_slice) {
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}},
                    {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2) -= source(Dim::X, 1, 3);
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -21.0);
    EXPECT_EQ(data[4], -22.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}},
                    {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2) -= source(Dim::X, 1, 3);
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], -11.0);
    EXPECT_EQ(data[2], -12.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -21.0);
    EXPECT_EQ(data[5], -22.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], 0.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}},
                    {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3) -= source(Dim::X, 1, 3);
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], -11.0);
    EXPECT_EQ(data[4], -12.0);
    EXPECT_EQ(data[5], 0.0);
    EXPECT_EQ(data[6], -21.0);
    EXPECT_EQ(data[7], -22.0);
    EXPECT_EQ(data[8], 0.0);
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}},
                    {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3) -= source(Dim::X, 1, 3);
    const auto data = target.get(Data::Value);
    EXPECT_EQ(data[0], 0.0);
    EXPECT_EQ(data[1], 0.0);
    EXPECT_EQ(data[2], 0.0);
    EXPECT_EQ(data[3], 0.0);
    EXPECT_EQ(data[4], -11.0);
    EXPECT_EQ(data[5], -12.0);
    EXPECT_EQ(data[6], 0.0);
    EXPECT_EQ(data[7], -21.0);
    EXPECT_EQ(data[8], -22.0);
  }
}

TEST(VariableSlice, slice_minus_lower_dimensional) {
  Variable target(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}});
  Variable source(Data::Value, {Dim::X, 2}, {1.0, 2.0});
  EXPECT_EQ(target(Dim::Y, 1, 2).dimensions(),
            (Dimensions{{Dim::Y, 1}, {Dim::X, 2}}));

  target(Dim::Y, 1, 2) -= source;

  const auto data = target.get(Data::Value);
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableSlice, variable_copy_from_slice) {
  const Variable source(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}},
                        {11, 12, 13, 21, 22, 23, 31, 32, 33});

  Variable target1(source(Dim::X, 0, 2)(Dim::Y, 0, 2));
  EXPECT_EQ(target1.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target1.get(Data::Value), {11, 12, 21, 22}));

  Variable target2(source(Dim::X, 1, 3)(Dim::Y, 0, 2));
  EXPECT_EQ(target2.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target2.get(Data::Value), {12, 13, 22, 23}));

  Variable target3(source(Dim::X, 0, 2)(Dim::Y, 1, 3));
  EXPECT_EQ(target3.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target3.get(Data::Value), {21, 22, 31, 32}));

  Variable target4(source(Dim::X, 1, 3)(Dim::Y, 1, 3));
  EXPECT_EQ(target4.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target4.get(Data::Value), {22, 23, 32, 33}));
}

TEST(VariableSlice, variable_assign_from_slice) {
  Variable target(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  const Variable source(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}},
                        {11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = source(Dim::X, 0, 2)(Dim::Y, 0, 2);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get(Data::Value), {11, 12, 21, 22}));

  target = source(Dim::X, 1, 3)(Dim::Y, 0, 2);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get(Data::Value), {12, 13, 22, 23}));

  target = source(Dim::X, 0, 2)(Dim::Y, 1, 3);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get(Data::Value), {21, 22, 31, 32}));

  target = source(Dim::X, 1, 3)(Dim::Y, 1, 3);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get(Data::Value), {22, 23, 32, 33}));
}

TEST(VariableSlice, variable_self_assign_via_slice) {
  Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}},
                  {11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = target(Dim::X, 1, 3)(Dim::Y, 1, 3);
  // Note: This test does not actually fail if self-assignment is broken. Had to
  // run address sanitizer to see that it is reading from free'ed memory.
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.get(Data::Value), {22, 23, 32, 33}));
}

TEST(VariableSlice, slice_assign_from_variable) {
  const Variable source(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}},
                        {11, 12, 21, 22});

  // We might want to mimick Python's __setitem__, but operator= would (and
  // should!?) assign the view contents, not the data.
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2).assign(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(
        equals(target.get(Data::Value), {11, 12, 0, 21, 22, 0, 0, 0, 0}));
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2).assign(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(
        equals(target.get(Data::Value), {0, 11, 12, 0, 21, 22, 0, 0, 0}));
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3).assign(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(
        equals(target.get(Data::Value), {0, 0, 0, 11, 12, 0, 21, 22, 0}));
  }
  {
    Variable target(Data::Value, {{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3).assign(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(
        equals(target.get(Data::Value), {0, 0, 0, 0, 11, 12, 0, 21, 22}));
  }
}

TEST(VariableSlice, slice_binary_operations) {
  Variable v(Data::Value, {{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  // Note: There does not seem to be a way to test whether this is using the
  // operators that convert the second argument to Variable (it should not), or
  // keep it as a view. See variable_benchmark.cpp for an attempt to verify
  // this.
  auto sum = v(Dim::X, 0) + v(Dim::X, 1);
  auto difference = v(Dim::X, 0) - v(Dim::X, 1);
  auto product = v(Dim::X, 0) * v(Dim::X, 1);
  auto ratio = v(Dim::X, 0) / v(Dim::X, 1);
  EXPECT_TRUE(equals(sum.get(Data::Value), {3, 7}));
  EXPECT_TRUE(equals(difference.get(Data::Value), {-1, -1}));
  EXPECT_TRUE(equals(product.get(Data::Value), {2, 12}));
  EXPECT_TRUE(equals(ratio.get(Data::Value), {1.0 / 2.0, 3.0 / 4.0}));
}

TEST(Variable, reshape) {
  const Variable var(Data::Value, {{Dim::X, 2}, {Dim::Y, 3}},
                     {1, 2, 3, 4, 5, 6});
  auto view = var.reshape({Dim::Row, 6});
  ASSERT_EQ(view.size(), 6);
  ASSERT_EQ(view.dimensions(), Dimensions({Dim::Row, 6}));
  EXPECT_TRUE(equals(view.get(Data::Value), {1, 2, 3, 4, 5, 6}));

  auto view2 = var.reshape({{Dim::Row, 3}, {Dim::Z, 2}});
  ASSERT_EQ(view2.size(), 6);
  ASSERT_EQ(view2.dimensions(), Dimensions({{Dim::Row, 3}, {Dim::Z, 2}}));
  EXPECT_TRUE(equals(view2.get(Data::Value), {1, 2, 3, 4, 5, 6}));
}

TEST(Variable, reshape_temporary) {
  const Variable var(Data::Value, {{Dim::X, 2}, {Dim::Row, 4}},
                     {1, 2, 3, 4, 5, 6, 7, 8});
  auto reshaped = sum(var, Dim::X).reshape({{Dim::Y, 2}, {Dim::Z, 2}});
  ASSERT_EQ(reshaped.size(), 4);
  ASSERT_EQ(reshaped.dimensions(), Dimensions({{Dim::Y, 2}, {Dim::Z, 2}}));
  EXPECT_TRUE(equals(reshaped.get(Data::Value), {6, 8, 10, 12}));

  // This is not a temporary, we get a view into `var`.
  EXPECT_EQ(typeid(decltype(std::move(var).reshape({}))),
            typeid(ConstVariableSlice));
}

TEST(Variable, reshape_fail) {
  Variable var(Data::Value, {{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
  EXPECT_THROW_MSG(var.reshape({Dim::Row, 5}), std::runtime_error,
                   "Cannot reshape to dimensions with different volume");
}

TEST(Variable, reshape_and_slice) {
  Variable var(Data::Value, {Dim::Spectrum, 16},
               {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});

  auto slice =
      var.reshape({{Dim::X, 4}, {Dim::Y, 4}})(Dim::X, 1, 3)(Dim::Y, 1, 3);
  EXPECT_TRUE(equals(slice.get(Data::Value), {6, 7, 10, 11}));

  Variable center =
      var.reshape({{Dim::X, 4}, {Dim::Y, 4}})(Dim::X, 1, 3)(Dim::Y, 1, 3)
          .reshape({Dim::Spectrum, 4});

  ASSERT_EQ(center.size(), 4);
  ASSERT_EQ(center.dimensions(), Dimensions({Dim::Spectrum, 4}));
  EXPECT_TRUE(equals(center.get(Data::Value), {6, 7, 10, 11}));
}

TEST(Variable, reshape_mutable) {
  Variable var(Data::Value, {{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
  const auto copy(var);

  auto view = var.reshape({Dim::Row, 6});
  view.get(Data::Value)[3] = 0;

  EXPECT_TRUE(equals(view.get(Data::Value), {1, 2, 3, 0, 5, 6}));
  EXPECT_TRUE(equals(var.get(Data::Value), {1, 2, 3, 0, 5, 6}));
  EXPECT_TRUE(equals(copy.get(Data::Value), {1, 2, 3, 4, 5, 6}));
}

TEST(Variable, reverse) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}}, {1, 2, 3, 4, 5, 6});
  Variable reverseX(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}},
                    {3, 2, 1, 6, 5, 4});
  Variable reverseY(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}},
                    {4, 5, 6, 1, 2, 3});

  EXPECT_EQ(reverse(var, Dim::X), reverseX);
  EXPECT_EQ(reverse(var, Dim::Y), reverseY);
}

TEST(Variable, access_typed_view) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}}, {1, 2, 3, 4, 5, 6});
  const auto values =
      getView<double>(var, {{Dim::Y, 2}, {Dim::Z, 4}, {Dim::X, 3}});
  ASSERT_EQ(values.size(), 24);

  for (const auto z : {0, 1, 2, 3}) {
    EXPECT_EQ(values[3 * z + 0], 1);
    EXPECT_EQ(values[3 * z + 1], 2);
    EXPECT_EQ(values[3 * z + 2], 3);
  }
  for (const auto z : {0, 1, 2, 3}) {
    EXPECT_EQ(values[12 + 3 * z + 0], 4);
    EXPECT_EQ(values[12 + 3 * z + 1], 5);
    EXPECT_EQ(values[12 + 3 * z + 2], 6);
  }
}

TEST(Variable, access_typed_view_edges) {
  // If a variable contains bin edges we want to "skip" the last edge. Say bins
  // is in direction Y:
  Variable var(Data::Value, {{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
  const auto values =
      getView<double>(var, {{Dim::Y, 2}, {Dim::Z, 4}, {Dim::X, 2}});
  ASSERT_EQ(values.size(), 16);

  for (const auto z : {0, 1, 2, 3}) {
    EXPECT_EQ(values[2 * z + 0], 1);
    EXPECT_EQ(values[2 * z + 1], 4);
  }
  for (const auto z : {0, 1, 2, 3}) {
    EXPECT_EQ(values[8 + 2 * z + 0], 2);
    EXPECT_EQ(values[8 + 2 * z + 1], 5);
  }
}

TEST(Variable, non_in_place_scalar_operations) {
  Variable var(Data::Value, {{Dim::X, 2}}, {1, 2});

  auto sum = var + 1;
  EXPECT_TRUE(equals(sum.get(Data::Value), {2, 3}));
  sum = 2 + var;
  EXPECT_TRUE(equals(sum.get(Data::Value), {3, 4}));

  auto diff = var - 1;
  EXPECT_TRUE(equals(diff.get(Data::Value), {0, 1}));
  diff = 2 - var;
  EXPECT_TRUE(equals(diff.get(Data::Value), {1, 0}));

  auto prod = var * 2;
  EXPECT_TRUE(equals(prod.get(Data::Value), {2, 4}));
  prod = 3 * var;
  EXPECT_TRUE(equals(prod.get(Data::Value), {3, 6}));

  auto ratio = var / 2;
  EXPECT_TRUE(equals(ratio.get(Data::Value), {1.0 / 2.0, 1.0}));
  ratio = 3 / var;
  EXPECT_TRUE(equals(ratio.get(Data::Value), {3.0, 1.5}));
}

TEST(VariableSlice, scalar_operations) {
  Variable var(Data::Value, {{Dim::Y, 2}, {Dim::X, 3}},
               {11, 12, 13, 21, 22, 23});

  var(Dim::X, 0) += 1;
  EXPECT_TRUE(equals(var.get(Data::Value), {12, 12, 13, 22, 22, 23}));
  var(Dim::Y, 1) += 1;
  EXPECT_TRUE(equals(var.get(Data::Value), {12, 12, 13, 23, 23, 24}));
  var(Dim::X, 1, 3) += 1;
  EXPECT_TRUE(equals(var.get(Data::Value), {12, 13, 14, 23, 24, 25}));
  var(Dim::X, 1) -= 1;
  EXPECT_TRUE(equals(var.get(Data::Value), {12, 12, 14, 23, 23, 25}));
  var(Dim::X, 2) *= 0;
  EXPECT_TRUE(equals(var.get(Data::Value), {12, 12, 0, 23, 23, 0}));
  var(Dim::Y, 0) /= 2;
  EXPECT_TRUE(equals(var.get(Data::Value), {6, 6, 0, 23, 23, 0}));
}

TEST(Variable, apply_unary_in_place) {
  Variable var(Data::Value, {Dim::X, 2}, {1.1, 2.2});
  var.transform_in_place<double>([](const double x) { return -x; });
  EXPECT_TRUE(equals(var.span<double>(), {-1.1, -2.2}));
}

TEST(Variable, apply_unary_implicit_conversion) {
  const auto var = makeVariable<float>(Data::Value, {Dim::X, 2}, {1.1, 2.2});
  // The functor returns double, so the output type is also double.
  auto out = var.transform<double, float>([](const double x) { return -x; });
  EXPECT_TRUE(equals(out.span<double>(), {-1.1f, -2.2f}));
}

TEST(Variable, apply_unary) {
  const auto varD = makeVariable<double>(Data::Value, {Dim::X, 2}, {1.1, 2.2});
  const auto varF = makeVariable<float>(Data::Value, {Dim::X, 2}, {1.1, 2.2});
  auto outD = varD.transform<double, float>([](const auto x) { return -x; });
  auto outF = varF.transform<double, float>([](const auto x) { return -x; });
  EXPECT_TRUE(equals(outD.span<double>(), {-1.1, -2.2}));
  EXPECT_TRUE(equals(outF.span<float>(), {-1.1f, -2.2f}));
}
