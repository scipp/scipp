// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "dimensions.h"
#include "tags.h"
#include "transform.h"
#include "variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(Variable, construct) {
  ASSERT_NO_THROW(makeVariable<double>(Dimensions(Dim::Tof, 2)));
  ASSERT_NO_THROW(makeVariable<double>(Dimensions(Dim::Tof, 2), 2));
  const auto a = makeVariable<double>(Dimensions(Dim::Tof, 2));
  const auto &data = a.span<double>();
  EXPECT_EQ(data.size(), 2);
}

TEST(Variable, construct_fail) {
  ASSERT_ANY_THROW(makeVariable<double>(Dimensions(), 2));
  ASSERT_ANY_THROW(makeVariable<double>(Dimensions(Dim::Tof, 1), 2));
  ASSERT_ANY_THROW(makeVariable<double>(Dimensions(Dim::Tof, 3), 2));
}

TEST(Variable, DISABLED_move) {
  auto var = makeVariable<double>({Dim::X, 2});
  Variable moved(std::move(var));
  // We need to define the behavior on move. Currently most methods will just
  // segfault, and we have no way of telling whether a Variable is in this
  // state.
  EXPECT_NE(var, moved);
}

TEST(Variable, makeVariable_custom_type) {
  auto doubles = makeVariable<double>({});
  auto floats = makeVariable<float>({});

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
  auto doubles = makeVariable<double>({Dim::X, 2}, {1, 2});
  auto ints = makeVariable<int32_t>({Dim::X, 2}, {1.1, 2.2});

  // Passed ints but uses default type based on tag.
  ASSERT_NO_THROW(doubles.span<double>());
  // Passed doubles but explicit type overrides.
  ASSERT_NO_THROW(ints.span<int32_t>());
}

TEST(Variable, dtype) {
  auto doubles = makeVariable<double>({});
  auto floats = makeVariable<float>({});
  EXPECT_EQ(doubles.dtype(), dtype<double>);
  EXPECT_NE(doubles.dtype(), dtype<float>);
  EXPECT_NE(floats.dtype(), dtype<double>);
  EXPECT_EQ(floats.dtype(), dtype<float>);
  EXPECT_EQ(doubles.dtype(), doubles.dtype());
  EXPECT_EQ(floats.dtype(), floats.dtype());
  EXPECT_NE(doubles.dtype(), floats.dtype());
}

TEST(Variable, span_references_Variable) {
  auto a = makeVariable<double>(Dimensions(Dim::Tof, 2));
  auto observer = a.span<double>();
  // This line does not compile, const-correctness works:
  // observer[0] = 1.0;

  // Note: This also has the "usual" problem of copy-on-write: This non-const
  // call can invalidate the references stored in observer if the underlying
  // data was shared.
  auto span = a.span<double>();

  EXPECT_EQ(span.size(), 2);
  span[0] = 1.0;
  EXPECT_EQ(observer[0], 1.0);
}

TEST(Variable, copy) {
  const auto a1 = makeVariable<double>({Dim::Tof, 2}, {1.1, 2.2});
  const auto &data1 = a1.span<double>();
  EXPECT_EQ(data1[0], 1.1);
  EXPECT_EQ(data1[1], 2.2);
  auto a2(a1);
  EXPECT_NE(&a1.span<double>()[0], &a2.span<double>()[0]);
  EXPECT_NE(&a1.span<double>()[0], &a2.span<double>()[0]);
  const auto &data2 = a2.span<double>();
  EXPECT_EQ(data2[0], 1.1);
  EXPECT_EQ(data2[1], 2.2);
}

TEST(Variable, operator_equals) {
  const auto a = makeVariable<double>({Dim::Tof, 2}, {1.1, 2.2});
  const auto a_copy(a);
  const auto b = makeVariable<double>({Dim::Tof, 2}, {1.1, 2.2});
  const auto diff1 = makeVariable<double>({Dim::Tof, 2}, {1.1, 2.1});
  const auto diff2 = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  auto diff3(a);
  diff3.setUnit(units::m);
  EXPECT_EQ(a, a);
  EXPECT_EQ(a, a_copy);
  EXPECT_EQ(a, b);
  EXPECT_EQ(b, a);
  EXPECT_FALSE(a == diff1);
  EXPECT_FALSE(a == diff2);
  EXPECT_FALSE(a == diff3);
}

TEST(Variable, operator_equals_mismatching_dtype) {
  auto a = makeVariable<double>({});
  auto b = makeVariable<float>({});
  EXPECT_NE(a, b);
}

TEST(Variable, operator_unary_minus) {
  const auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  auto b = -a;
  EXPECT_EQ(a.span<double>()[0], 1.1);
  EXPECT_EQ(a.span<double>()[1], 2.2);
  EXPECT_EQ(b.span<double>()[0], -1.1);
  EXPECT_EQ(b.span<double>()[1], -2.2);
}

TEST(VariableSlice, unary_minus) {
  const auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  auto b = -a(Dim::X, 1);
  EXPECT_EQ(a.span<double>()[0], 1.1);
  EXPECT_EQ(a.span<double>()[1], 2.2);
  EXPECT_EQ(b.span<double>()[0], -2.2);
}

TEST(Variable, operator_plus_equal) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});

  ASSERT_NO_THROW(a += a);
  EXPECT_EQ(a.span<double>()[0], 2.2);
  EXPECT_EQ(a.span<double>()[1], 4.4);
}

TEST(Variable, operator_plus_equal_automatic_broadcast_of_rhs) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});

  auto fewer_dimensions = makeVariable<double>({}, {1.0});

  ASSERT_NO_THROW(a += fewer_dimensions);
  EXPECT_EQ(a.span<double>()[0], 2.1);
  EXPECT_EQ(a.span<double>()[1], 3.2);
}

TEST(Variable, operator_plus_equal_transpose) {
  auto a = makeVariable<double>(Dimensions({{Dim::Y, 3}, {Dim::X, 2}}),
                                {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  auto transpose = makeVariable<double>(Dimensions({{Dim::X, 2}, {Dim::Y, 3}}),
                                        {1.0, 3.0, 5.0, 2.0, 4.0, 6.0});

  EXPECT_NO_THROW(a += transpose);
  EXPECT_EQ(a.span<double>()[0], 2.0);
  EXPECT_EQ(a.span<double>()[1], 4.0);
  EXPECT_EQ(a.span<double>()[2], 6.0);
  EXPECT_EQ(a.span<double>()[3], 8.0);
  EXPECT_EQ(a.span<double>()[4], 10.0);
  EXPECT_EQ(a.span<double>()[5], 12.0);
}

TEST(Variable, operator_plus_equal_different_dimensions) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});

  auto different_dimensions = makeVariable<double>({Dim::Y, 2}, {1.1, 2.2});
  EXPECT_THROW_MSG(a += different_dimensions, std::runtime_error,
                   "Expected {{Dim::X, 2}} to contain {{Dim::Y, 2}}.");
}

TEST(Variable, operator_plus_equal_different_unit) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});

  auto different_unit(a);
  different_unit.setUnit(units::m);
  EXPECT_THROW_MSG(a += different_unit, except::UnitMismatchError,
                   "Expected dimensionless to be equal to m.");
}

TEST(Variable, operator_plus_equal_non_arithmetic_type) {
  auto a = makeVariable<std::string>({Dim::X, 1}, {std::string("test")});
  EXPECT_THROW(a += a, except::TypeError);
}

TEST(Variable, operator_plus_equal_different_variables_different_element_type) {
  auto a = makeVariable<double>({Dim::X, 1}, {1.0});
  auto b = makeVariable<int64_t>({Dim::X, 1}, {2});
  EXPECT_THROW(a += b, except::TypeError);
}

TEST(Variable, operator_plus_equal_different_variables_same_element_type) {
  auto a = makeVariable<double>({Dim::X, 1}, {1.0});
  auto b = makeVariable<double>({Dim::X, 1}, {2.0});
  EXPECT_NO_THROW(a += b);
  EXPECT_EQ(a.span<double>()[0], 3.0);
}

TEST(Variable, operator_plus_equal_scalar) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});

  EXPECT_NO_THROW(a += 1.0);
  EXPECT_EQ(a.span<double>()[0], 2.1);
  EXPECT_EQ(a.span<double>()[1], 3.2);
}

TEST(Variable, operator_plus_equal_custom_type) {
  auto a = makeVariable<float>({Dim::X, 2}, {1.1f, 2.2f});

  EXPECT_NO_THROW(a += a);
  EXPECT_EQ(a.span<float>()[0], 2.2f);
  EXPECT_EQ(a.span<float>()[1], 4.4f);
}

TEST(Variable, operator_times_equal) {
  auto a = makeVariable<double>({Dim::X, 2}, {2.0, 3.0});

  EXPECT_EQ(a.unit(), units::m);
  EXPECT_NO_THROW(a *= a);
  EXPECT_EQ(a.span<double>()[0], 4.0);
  EXPECT_EQ(a.span<double>()[1], 9.0);
  EXPECT_EQ(a.unit(), units::m * units::m);
}

TEST(Variable, operator_times_equal_scalar) {
  auto a = makeVariable<double>({Dim::X, 2}, {2.0, 3.0});

  EXPECT_EQ(a.unit(), units::m);
  EXPECT_NO_THROW(a *= 2.0);
  EXPECT_EQ(a.span<double>()[0], 4.0);
  EXPECT_EQ(a.span<double>()[1], 6.0);
  EXPECT_EQ(a.unit(), units::m);
}

TEST(Variable, operator_times_can_broadcast) {
  auto a = makeVariable<double>({Dim::X, 2}, {0.5, 1.5});
  auto b = makeVariable<double>({Dim::Y, 2}, {2.0, 3.0});

  auto ab = a * b;
  auto reference =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 3.0, 1.5, 4.5});
  EXPECT_EQ(ab, reference);
}

TEST(Variable, operator_divide_equal) {
  auto a = makeVariable<double>({Dim::X, 2}, {2.0, 3.0});
  auto b = makeVariable<double>({}, {2.0});
  b.setUnit(units::m);

  EXPECT_NO_THROW(a /= b);
  EXPECT_EQ(a.span<double>()[0], 1.0);
  EXPECT_EQ(a.span<double>()[1], 1.5);
  EXPECT_EQ(a.unit(), units::dimensionless / units::m);
}

TEST(Variable, operator_divide_equal_self) {
  auto a = makeVariable<double>({Dim::X, 2}, {2.0, 3.0});

  EXPECT_EQ(a.unit(), units::m);
  EXPECT_NO_THROW(a /= a);
  EXPECT_EQ(a.span<double>()[0], 1.0);
  EXPECT_EQ(a.span<double>()[1], 1.0);
  EXPECT_EQ(a.unit(), units::dimensionless);
}

TEST(Variable, operator_divide_equal_scalar) {
  auto a = makeVariable<double>({Dim::X, 2}, {2.0, 4.0});

  EXPECT_EQ(a.unit(), units::m);
  EXPECT_NO_THROW(a /= 2.0);
  EXPECT_EQ(a.span<double>()[0], 1.0);
  EXPECT_EQ(a.span<double>()[1], 2.0);
  EXPECT_EQ(a.unit(), units::m);
}

TEST(Variable, operator_divide_scalar_double) {
  const auto a = makeVariable<double>({Dim::X, 2}, {2.0, 4.0});
  const auto result = 1.111 / a;
  EXPECT_EQ(result.span<double>()[0], 1.111 / 2.0);
  EXPECT_EQ(result.span<double>()[1], 1.111 / 4.0);
  EXPECT_EQ(result.unit(), units::dimensionless / units::m);
}

TEST(Variable, operator_divide_scalar_float) {
  const auto a = makeVariable<float>({Dim::X, 2}, {2.0, 4.0});
  const auto result = 1.111 / a;
  EXPECT_EQ(result.span<float>()[0], 1.111f / 2.0f);
  EXPECT_EQ(result.span<float>()[1], 1.111f / 4.0f);
  EXPECT_EQ(result.unit(), units::dimensionless / units::m);
}

TEST(Variable, setSlice) {
  Dimensions dims(Dim::Tof, 1);
  const auto parent = makeVariable<double>(
      Dimensions({{Dim::X, 4}, {Dim::Y, 2}, {Dim::Z, 3}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});
  const auto empty = makeVariable<double>(
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
  const auto parent = makeVariable<double>(
      Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 4}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const scipp::index index : {0, 1, 2, 3}) {
    Variable sliceX = parent(Dim::X, index);
    ASSERT_EQ(sliceX.dimensions(), Dimensions({{Dim::Z, 3}, {Dim::Y, 2}}));
    auto base = static_cast<double>(index);
    EXPECT_EQ(sliceX.span<double>()[0], base + 1.0);
    EXPECT_EQ(sliceX.span<double>()[1], base + 5.0);
    EXPECT_EQ(sliceX.span<double>()[2], base + 9.0);
    EXPECT_EQ(sliceX.span<double>()[3], base + 13.0);
    EXPECT_EQ(sliceX.span<double>()[4], base + 17.0);
    EXPECT_EQ(sliceX.span<double>()[5], base + 21.0);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceY = parent(Dim::Y, index);
    ASSERT_EQ(sliceY.dimensions(), Dimensions({{Dim::Z, 3}, {Dim::X, 4}}));
    const auto &data = sliceY.span<double>();
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
    const auto &data = sliceZ.span<double>();
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }
}

TEST(Variable, slice_range) {
  Dimensions dims(Dim::Tof, 1);
  const auto parent = makeVariable<double>(
      Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 4}}),
      {1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,  10.0, 11.0, 12.0,
       13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0, 23.0, 24.0});

  for (const scipp::index index : {0, 1, 2, 3}) {
    Variable sliceX = parent(Dim::X, index, index + 1);
    ASSERT_EQ(sliceX.dimensions(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 1}}));
    EXPECT_EQ(sliceX.span<double>()[0], index + 1.0);
    EXPECT_EQ(sliceX.span<double>()[1], index + 5.0);
    EXPECT_EQ(sliceX.span<double>()[2], index + 9.0);
    EXPECT_EQ(sliceX.span<double>()[3], index + 13.0);
    EXPECT_EQ(sliceX.span<double>()[4], index + 17.0);
    EXPECT_EQ(sliceX.span<double>()[5], index + 21.0);
  }

  for (const scipp::index index : {0, 1, 2}) {
    Variable sliceX = parent(Dim::X, index, index + 2);
    ASSERT_EQ(sliceX.dimensions(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 2}}));
    EXPECT_EQ(sliceX.span<double>()[0], index + 1.0);
    EXPECT_EQ(sliceX.span<double>()[1], index + 2.0);
    EXPECT_EQ(sliceX.span<double>()[2], index + 5.0);
    EXPECT_EQ(sliceX.span<double>()[3], index + 6.0);
    EXPECT_EQ(sliceX.span<double>()[4], index + 9.0);
    EXPECT_EQ(sliceX.span<double>()[5], index + 10.0);
    EXPECT_EQ(sliceX.span<double>()[6], index + 13.0);
    EXPECT_EQ(sliceX.span<double>()[7], index + 14.0);
    EXPECT_EQ(sliceX.span<double>()[8], index + 17.0);
    EXPECT_EQ(sliceX.span<double>()[9], index + 18.0);
    EXPECT_EQ(sliceX.span<double>()[10], index + 21.0);
    EXPECT_EQ(sliceX.span<double>()[11], index + 22.0);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceY = parent(Dim::Y, index, index + 1);
    ASSERT_EQ(sliceY.dimensions(),
              Dimensions({{Dim::Z, 3}, {Dim::Y, 1}, {Dim::X, 4}}));
    const auto &data = sliceY.span<double>();
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
    const auto &data = sliceZ.span<double>();
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
  }

  for (const scipp::index index : {0, 1}) {
    Variable sliceZ = parent(Dim::Z, index, index + 2);
    ASSERT_EQ(sliceZ.dimensions(),
              Dimensions({{Dim::Z, 2}, {Dim::Y, 2}, {Dim::X, 4}}));
    const auto &data = sliceZ.span<double>();
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[xy], 1.0 + xy + 8 * index);
    for (scipp::index xy = 0; xy < 8; ++xy)
      EXPECT_EQ(data[8 + xy], 1.0 + 8 + xy + 8 * index);
  }
}

TEST(Variable, concatenate) {
  Dimensions dims(Dim::Tof, 1);
  auto a = makeVariable<double>(dims, {1.0});
  auto b = makeVariable<double>(dims, {2.0});
  a.setUnit(units::m);
  b.setUnit(units::m);
  auto ab = concatenate(a, b, Dim::Tof);
  ASSERT_EQ(ab.size(), 2);
  EXPECT_EQ(ab.unit(), units::Unit(units::m));
  const auto &data = ab.span<double>();
  EXPECT_EQ(data[0], 1.0);
  EXPECT_EQ(data[1], 2.0);
  auto ba = concatenate(b, a, Dim::Tof);
  const auto abba = concatenate(ab, ba, Dim::Q);
  ASSERT_EQ(abba.size(), 4);
  EXPECT_EQ(abba.dimensions().count(), 2);
  const auto &data2 = abba.span<double>();
  EXPECT_EQ(data2[0], 1.0);
  EXPECT_EQ(data2[1], 2.0);
  EXPECT_EQ(data2[2], 2.0);
  EXPECT_EQ(data2[3], 1.0);
  const auto ababbaba = concatenate(abba, abba, Dim::Tof);
  ASSERT_EQ(ababbaba.size(), 8);
  const auto &data3 = ababbaba.span<double>();
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
  const auto &data4 = abbaabba.span<double>();
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
  auto a = makeVariable<double>({Dim::X, 1}, {1.0});
  auto aa = concatenate(a, a, Dim::X);
  EXPECT_NO_THROW(concatenate(aa, a, Dim::X));
}

TEST(Variable, concatenate_slice_with_volume) {
  auto a = makeVariable<double>({Dim::X, 1}, {1.0});
  auto aa = concatenate(a, a, Dim::X);
  EXPECT_NO_THROW(concatenate(a, aa, Dim::X));
}

TEST(Variable, concatenate_fail) {
  Dimensions dims(Dim::Tof, 1);
  auto a = makeVariable<double>(dims, {1.0});
  auto b = makeVariable<double>(dims, {2.0});
  auto c = makeVariable<double>(dims, {2.0});
  EXPECT_THROW_MSG(concatenate(a, c, Dim::Tof), std::runtime_error,
                   "Cannot concatenate Variables: Data types do not match.");
  auto aa = concatenate(a, a, Dim::Tof);
  EXPECT_THROW_MSG(
      concatenate(a, aa, Dim::Q), std::runtime_error,
      "Cannot concatenate Variables: Dimension extents do not match.");
}

TEST(Variable, concatenate_unit_fail) {
  Dimensions dims(Dim::X, 1);
  auto a = makeVariable<double>(dims, {1.0});
  auto b(a);
  EXPECT_NO_THROW(concatenate(a, b, Dim::X));
  a.setUnit(units::m);
  EXPECT_THROW_MSG(concatenate(a, b, Dim::X), std::runtime_error,
                   "Cannot concatenate Variables: Units do not match.");
  b.setUnit(units::m);
  EXPECT_NO_THROW(concatenate(a, b, Dim::X));
}

TEST(Variable, rebin) {
  auto var = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  var.setUnit(units::counts);
  const auto oldEdge = makeVariable<double>({Dim::X, 3}, {1.0, 2.0, 3.0});
  const auto newEdge = makeVariable<double>({Dim::X, 2}, {1.0, 3.0});
  auto rebinned = rebin(var, oldEdge, newEdge);
  ASSERT_EQ(rebinned.dimensions().count(), 1);
  ASSERT_EQ(rebinned.dimensions().volume(), 1);
  ASSERT_EQ(rebinned.span<double>().size(), 1);
  EXPECT_EQ(rebinned.span<double>()[0], 3.0);
}

TEST(Variable, sum) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto sumX = sum(var, Dim::X);
  ASSERT_EQ(sumX.dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_TRUE(equals(sumX.span<double>(), {3.0, 7.0}));
  auto sumY = sum(var, Dim::Y);
  ASSERT_EQ(sumY.dimensions(), (Dimensions{Dim::X, 2}));
  EXPECT_TRUE(equals(sumY.span<double>(), {4.0, 6.0}));
}

TEST(Variable, mean) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto meanX = mean(var, Dim::X);
  ASSERT_EQ(meanX.dimensions(), (Dimensions{Dim::Y, 2}));
  EXPECT_TRUE(equals(meanX.span<double>(), {1.5, 3.5}));
  auto meanY = mean(var, Dim::Y);
  ASSERT_EQ(meanY.dimensions(), (Dimensions{Dim::X, 2}));
  EXPECT_TRUE(equals(meanY.span<double>(), {2.0, 3.0}));
}

TEST(Variable, abs_of_scalar) {
  auto reference =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, -2.0, -3.0, 4.0});
  EXPECT_EQ(abs(var), reference);
}

TEST(Variable, norm_of_vector) {
  auto reference =
      makeVariable<double>({Dim::X, 3}, {sqrt(2.0), sqrt(2.0), 2.0});
  auto var = makeVariable<Eigen::Vector3d>(
      {Dim::X, 3}, {Eigen::Vector3d{1, 0, -1}, Eigen::Vector3d{1, 1, 0},
                    Eigen::Vector3d{0, 0, -2}});
  EXPECT_EQ(norm(var), reference);
}

TEST(Variable, sqrt_double) {
  // TODO Currently comparisons of variables do not provide special handling of
  // NaN, so sqrt of negative values will lead variables that are never equal.
  auto reference = makeVariable<double>({Dim::X, 2}, {1, 2});
  reference.setUnit(units::m);
  auto var = makeVariable<double>({Dim::X, 2}, {1, 4});
  var.setUnit(units::m * units::m);
  EXPECT_EQ(sqrt(var), reference);
}

TEST(Variable, sqrt_float) {
  auto reference = makeVariable<float>({Dim::X, 2}, {1, 2});
  reference.setUnit(units::m);
  auto var = makeVariable<float>({Dim::X, 2}, {1, 4});
  var.setUnit(units::m * units::m);
  EXPECT_EQ(sqrt(var), reference);
}

TEST(Variable, broadcast) {
  auto reference = makeVariable<double>({{Dim::Z, 3}, {Dim::Y, 2}, {Dim::X, 2}},
                                        {1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4});
  auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});

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
  auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  EXPECT_THROW_MSG(broadcast(var, {Dim::X, 3}), except::DimensionLengthError,
                   "Expected dimension to be in {{Dim::Y, 2}, {Dim::X, 2}}, "
                   "got Dim::X with mismatching length 3.");
}

TEST(VariableSlice, full_const_view) {
  const auto var = makeVariable<double>({{Dim::X, 3}});
  ConstVariableSlice view(var);
  EXPECT_EQ(var.span<double>().data(), view.span<double>().data());
}

TEST(VariableSlice, full_mutable_view) {
  auto var = makeVariable<double>({{Dim::X, 3}});
  VariableSlice view(var);
  EXPECT_EQ(var.span<double>().data(), view.span<double>().data());
  EXPECT_EQ(var.span<double>().data(), view.span<double>().data());
}

TEST(VariableSlice, strides) {
  auto var = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
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

  auto var3D = makeVariable<double>({{Dim::Z, 4}, {Dim::Y, 3}, {Dim::X, 2}});
  EXPECT_EQ(var3D(Dim::X, 0, 1)(Dim::Z, 0, 1).strides(),
            (std::vector<scipp::index>{6, 2, 1}));
}

TEST(VariableSlice, get) {
  const auto var = makeVariable<double>({Dim::X, 3}, {1, 2, 3});
  EXPECT_EQ(var(Dim::X, 1, 2).span<double>()[0], 2.0);
}

TEST(VariableSlice, slicing_does_not_transpose) {
  auto var = makeVariable<double>({{Dim::X, 3}, {Dim::Y, 3}});
  Dimensions expected{{Dim::X, 1}, {Dim::Y, 1}};
  EXPECT_EQ(var(Dim::X, 1, 2)(Dim::Y, 1, 2).dimensions(), expected);
  EXPECT_EQ(var(Dim::Y, 1, 2)(Dim::X, 1, 2).dimensions(), expected);
}

TEST(VariableSlice, minus_equals_failures) {
  auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 2}}, {1.0, 2.0, 3.0, 4.0});

  EXPECT_THROW_MSG(var -= var(Dim::X, 0, 1), std::runtime_error,
                   "Expected {{Dim::X, 2}, {Dim::Y, 2}} to contain {{Dim::X, "
                   "1}, {Dim::Y, 2}}.");
}

TEST(VariableSlice, self_overlapping_view_operation) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});

  var -= var(Dim::Y, 0);
  const auto data = var.span<double>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  // This is the critical part: After subtracting for y=0 the view points to
  // data containing 0.0, so subsequently the subtraction would have no effect
  // if self-overlap was not taken into account by the implementation.
  EXPECT_EQ(data[2], 2.0);
  EXPECT_EQ(data[3], 2.0);
}

TEST(VariableSlice, minus_equals_slice_const_outer) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  const auto copy(var);

  var -= copy(Dim::Y, 0);
  const auto data = var.span<double>();
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
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::Y, 0);
  const auto data = var.span<double>();
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
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::X, 0);
  const auto data = var.span<double>();
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
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});
  auto copy(var);

  var -= copy(Dim::X, 1)(Dim::Y, 1);
  const auto data = var.span<double>();
  EXPECT_EQ(data[0], -3.0);
  EXPECT_EQ(data[1], -2.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], 0.0);
}

TEST(VariableSlice, minus_equals_nontrivial_slices) {
  auto source = makeVariable<double>(
      {{Dim::Y, 3}, {Dim::X, 3}},
      {11.0, 12.0, 13.0, 21.0, 22.0, 23.0, 31.0, 32.0, 33.0});
  {
    auto target = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 0, 2)(Dim::Y, 0, 2);
    const auto data = target.span<double>();
    EXPECT_EQ(data[0], -11.0);
    EXPECT_EQ(data[1], -12.0);
    EXPECT_EQ(data[2], -21.0);
    EXPECT_EQ(data[3], -22.0);
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 1, 3)(Dim::Y, 0, 2);
    const auto data = target.span<double>();
    EXPECT_EQ(data[0], -12.0);
    EXPECT_EQ(data[1], -13.0);
    EXPECT_EQ(data[2], -22.0);
    EXPECT_EQ(data[3], -23.0);
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 0, 2)(Dim::Y, 1, 3);
    const auto data = target.span<double>();
    EXPECT_EQ(data[0], -21.0);
    EXPECT_EQ(data[1], -22.0);
    EXPECT_EQ(data[2], -31.0);
    EXPECT_EQ(data[3], -32.0);
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}});
    target -= source(Dim::X, 1, 3)(Dim::Y, 1, 3);
    const auto data = target.span<double>();
    EXPECT_EQ(data[0], -22.0);
    EXPECT_EQ(data[1], -23.0);
    EXPECT_EQ(data[2], -32.0);
    EXPECT_EQ(data[3], -33.0);
  }
}

TEST(VariableSlice, slice_inner_minus_equals) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});

  var(Dim::X, 0) -= var(Dim::X, 1);
  const auto data = var.span<double>();
  EXPECT_EQ(data[0], -1.0);
  EXPECT_EQ(data[1], 2.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableSlice, slice_outer_minus_equals) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1.0, 2.0, 3.0, 4.0});

  var(Dim::Y, 0) -= var(Dim::Y, 1);
  const auto data = var.span<double>();
  EXPECT_EQ(data[0], -2.0);
  EXPECT_EQ(data[1], -2.0);
  EXPECT_EQ(data[2], 3.0);
  EXPECT_EQ(data[3], 4.0);
}

TEST(VariableSlice, nontrivial_slice_minus_equals) {
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2) -= source;
    const auto data = target.span<double>();
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
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2) -= source;
    const auto data = target.span<double>();
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
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3) -= source;
    const auto data = target.span<double>();
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
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}},
                                       {11.0, 12.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3) -= source;
    const auto data = target.span<double>();
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
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}},
                                       {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2) -= source(Dim::X, 1, 3);
    const auto data = target.span<double>();
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
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}},
                                       {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2) -= source(Dim::X, 1, 3);
    const auto data = target.span<double>();
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
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}},
                                       {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3) -= source(Dim::X, 1, 3);
    const auto data = target.span<double>();
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
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    auto source = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}},
                                       {666.0, 11.0, 12.0, 666.0, 21.0, 22.0});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3) -= source(Dim::X, 1, 3);
    const auto data = target.span<double>();
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
  auto target = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}});
  auto source = makeVariable<double>({Dim::X, 2}, {1.0, 2.0});
  EXPECT_EQ(target(Dim::Y, 1, 2).dimensions(),
            (Dimensions{{Dim::Y, 1}, {Dim::X, 2}}));

  target(Dim::Y, 1, 2) -= source;

  const auto data = target.span<double>();
  EXPECT_EQ(data[0], 0.0);
  EXPECT_EQ(data[1], 0.0);
  EXPECT_EQ(data[2], -1.0);
  EXPECT_EQ(data[3], -2.0);
}

TEST(VariableSlice, variable_copy_from_slice) {
  const auto source = makeVariable<double>(
      {{Dim::Y, 3}, {Dim::X, 3}}, {11, 12, 13, 21, 22, 23, 31, 32, 33});

  Variable target1(source(Dim::X, 0, 2)(Dim::Y, 0, 2));
  EXPECT_EQ(target1.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target1.span<double>(), {11, 12, 21, 22}));

  Variable target2(source(Dim::X, 1, 3)(Dim::Y, 0, 2));
  EXPECT_EQ(target2.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target2.span<double>(), {12, 13, 22, 23}));

  Variable target3(source(Dim::X, 0, 2)(Dim::Y, 1, 3));
  EXPECT_EQ(target3.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target3.span<double>(), {21, 22, 31, 32}));

  Variable target4(source(Dim::X, 1, 3)(Dim::Y, 1, 3));
  EXPECT_EQ(target4.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target4.span<double>(), {22, 23, 32, 33}));
}

TEST(VariableSlice, variable_assign_from_slice) {
  auto target = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  const auto source = makeVariable<double>(
      {{Dim::Y, 3}, {Dim::X, 3}}, {11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = source(Dim::X, 0, 2)(Dim::Y, 0, 2);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.span<double>(), {11, 12, 21, 22}));

  target = source(Dim::X, 1, 3)(Dim::Y, 0, 2);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.span<double>(), {12, 13, 22, 23}));

  target = source(Dim::X, 0, 2)(Dim::Y, 1, 3);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.span<double>(), {21, 22, 31, 32}));

  target = source(Dim::X, 1, 3)(Dim::Y, 1, 3);
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.span<double>(), {22, 23, 32, 33}));
}

TEST(VariableSlice, variable_self_assign_via_slice) {
  auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}},
                                     {11, 12, 13, 21, 22, 23, 31, 32, 33});

  target = target(Dim::X, 1, 3)(Dim::Y, 1, 3);
  // Note: This test does not actually fail if self-assignment is broken. Had to
  // run address sanitizer to see that it is reading from free'ed memory.
  EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 2}, {Dim::X, 2}}));
  EXPECT_TRUE(equals(target.span<double>(), {22, 23, 32, 33}));
}

TEST(VariableSlice, slice_assign_from_variable) {
  const auto source =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {11, 12, 21, 22});

  // We might want to mimick Python's __setitem__, but operator= would (and
  // should!?) assign the view contents, not the data.
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 0, 2)(Dim::Y, 0, 2).assign(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(equals(target.span<double>(), {11, 12, 0, 21, 22, 0, 0, 0, 0}));
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 1, 3)(Dim::Y, 0, 2).assign(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(equals(target.span<double>(), {0, 11, 12, 0, 21, 22, 0, 0, 0}));
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 0, 2)(Dim::Y, 1, 3).assign(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(equals(target.span<double>(), {0, 0, 0, 11, 12, 0, 21, 22, 0}));
  }
  {
    auto target = makeVariable<double>({{Dim::Y, 3}, {Dim::X, 3}});
    target(Dim::X, 1, 3)(Dim::Y, 1, 3).assign(source);
    EXPECT_EQ(target.dimensions(), (Dimensions{{Dim::Y, 3}, {Dim::X, 3}}));
    EXPECT_TRUE(equals(target.span<double>(), {0, 0, 0, 0, 11, 12, 0, 21, 22}));
  }
}

TEST(VariableSlice, slice_binary_operations) {
  auto v = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 2}}, {1, 2, 3, 4});
  // Note: There does not seem to be a way to test whether this is using the
  // operators that convert the second argument to Variable (it should not), or
  // keep it as a view. See variable_benchmark.cpp for an attempt to verify
  // this.
  auto sum = v(Dim::X, 0) + v(Dim::X, 1);
  auto difference = v(Dim::X, 0) - v(Dim::X, 1);
  auto product = v(Dim::X, 0) * v(Dim::X, 1);
  auto ratio = v(Dim::X, 0) / v(Dim::X, 1);
  EXPECT_TRUE(equals(sum.span<double>(), {3, 7}));
  EXPECT_TRUE(equals(difference.span<double>(), {-1, -1}));
  EXPECT_TRUE(equals(product.span<double>(), {2, 12}));
  EXPECT_TRUE(equals(ratio.span<double>(), {1.0 / 2.0, 3.0 / 4.0}));
}

TEST(Variable, reshape) {
  const auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
  auto view = var.reshape({Dim::Row, 6});
  ASSERT_EQ(view.size(), 6);
  ASSERT_EQ(view.dimensions(), Dimensions({Dim::Row, 6}));
  EXPECT_TRUE(equals(view.span<double>(), {1, 2, 3, 4, 5, 6}));

  auto view2 = var.reshape({{Dim::Row, 3}, {Dim::Z, 2}});
  ASSERT_EQ(view2.size(), 6);
  ASSERT_EQ(view2.dimensions(), Dimensions({{Dim::Row, 3}, {Dim::Z, 2}}));
  EXPECT_TRUE(equals(view2.span<double>(), {1, 2, 3, 4, 5, 6}));
}

TEST(Variable, reshape_temporary) {
  const auto var = makeVariable<double>({{Dim::X, 2}, {Dim::Row, 4}},
                                        {1, 2, 3, 4, 5, 6, 7, 8});
  auto reshaped = sum(var, Dim::X).reshape({{Dim::Y, 2}, {Dim::Z, 2}});
  ASSERT_EQ(reshaped.size(), 4);
  ASSERT_EQ(reshaped.dimensions(), Dimensions({{Dim::Y, 2}, {Dim::Z, 2}}));
  EXPECT_TRUE(equals(reshaped.span<double>(), {6, 8, 10, 12}));

  // This is not a temporary, we get a view into `var`.
  EXPECT_EQ(typeid(decltype(std::move(var).reshape({}))),
            typeid(ConstVariableSlice));
}

TEST(Variable, reshape_fail) {
  auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
  EXPECT_THROW_MSG(var.reshape({Dim::Row, 5}), std::runtime_error,
                   "Cannot reshape to dimensions with different volume");
}

TEST(Variable, reshape_and_slice) {
  auto var =
      makeVariable<double>({Dim::Spectrum, 16}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                                 11, 12, 13, 14, 15, 16});

  auto slice =
      var.reshape({{Dim::X, 4}, {Dim::Y, 4}})(Dim::X, 1, 3)(Dim::Y, 1, 3);
  EXPECT_TRUE(equals(slice.span<double>(), {6, 7, 10, 11}));

  Variable center =
      var.reshape({{Dim::X, 4}, {Dim::Y, 4}})(Dim::X, 1, 3)(Dim::Y, 1, 3)
          .reshape({Dim::Spectrum, 4});

  ASSERT_EQ(center.size(), 4);
  ASSERT_EQ(center.dimensions(), Dimensions({Dim::Spectrum, 4}));
  EXPECT_TRUE(equals(center.span<double>(), {6, 7, 10, 11}));
}

TEST(Variable, reshape_mutable) {
  auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
  const auto copy(var);

  auto view = var.reshape({Dim::Row, 6});
  view.span<double>()[3] = 0;

  EXPECT_TRUE(equals(view.span<double>(), {1, 2, 3, 0, 5, 6}));
  EXPECT_TRUE(equals(var.span<double>(), {1, 2, 3, 0, 5, 6}));
  EXPECT_TRUE(equals(copy.span<double>(), {1, 2, 3, 4, 5, 6}));
}

TEST(Variable, reverse) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, {1, 2, 3, 4, 5, 6});
  auto reverseX =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, {3, 2, 1, 6, 5, 4});
  auto reverseY =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, {4, 5, 6, 1, 2, 3});

  EXPECT_EQ(reverse(var, Dim::X), reverseX);
  EXPECT_EQ(reverse(var, Dim::Y), reverseY);
}

TEST(Variable, access_typed_view) {
  auto var =
      makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, {1, 2, 3, 4, 5, 6});
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
  auto var =
      makeVariable<double>({{Dim::X, 2}, {Dim::Y, 3}}, {1, 2, 3, 4, 5, 6});
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
  auto var = makeVariable<double>({{Dim::X, 2}}, {1, 2});

  auto sum = var + 1;
  EXPECT_TRUE(equals(sum.span<double>(), {2, 3}));
  sum = 2 + var;
  EXPECT_TRUE(equals(sum.span<double>(), {3, 4}));

  auto diff = var - 1;
  EXPECT_TRUE(equals(diff.span<double>(), {0, 1}));
  diff = 2 - var;
  EXPECT_TRUE(equals(diff.span<double>(), {1, 0}));

  auto prod = var * 2;
  EXPECT_TRUE(equals(prod.span<double>(), {2, 4}));
  prod = 3 * var;
  EXPECT_TRUE(equals(prod.span<double>(), {3, 6}));

  auto ratio = var / 2;
  EXPECT_TRUE(equals(ratio.span<double>(), {1.0 / 2.0, 1.0}));
  ratio = 3 / var;
  EXPECT_TRUE(equals(ratio.span<double>(), {3.0, 1.5}));
}

TEST(VariableSlice, scalar_operations) {
  auto var = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}},
                                  {11, 12, 13, 21, 22, 23});

  var(Dim::X, 0) += 1;
  EXPECT_TRUE(equals(var.span<double>(), {12, 12, 13, 22, 22, 23}));
  var(Dim::Y, 1) += 1;
  EXPECT_TRUE(equals(var.span<double>(), {12, 12, 13, 23, 23, 24}));
  var(Dim::X, 1, 3) += 1;
  EXPECT_TRUE(equals(var.span<double>(), {12, 13, 14, 23, 24, 25}));
  var(Dim::X, 1) -= 1;
  EXPECT_TRUE(equals(var.span<double>(), {12, 12, 14, 23, 23, 25}));
  var(Dim::X, 2) *= 0;
  EXPECT_TRUE(equals(var.span<double>(), {12, 12, 0, 23, 23, 0}));
  var(Dim::Y, 0) /= 2;
  EXPECT_TRUE(equals(var.span<double>(), {6, 6, 0, 23, 23, 0}));
}

TEST(Variable, apply_unary_in_place) {
  auto var = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  transform_in_place<double>(var, [](const double x) { return -x; });
  EXPECT_TRUE(equals(var.span<double>(), {-1.1, -2.2}));
}

TEST(Variable, apply_unary_implicit_conversion) {
  const auto var = makeVariable<float>({Dim::X, 2}, {1.1, 2.2});
  // The functor returns double, so the output type is also double.
  auto out = transform<double, float>(var, [](const double x) { return -x; });
  EXPECT_TRUE(equals(out.span<double>(), {-1.1f, -2.2f}));
}

TEST(Variable, apply_unary) {
  const auto varD = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto varF = makeVariable<float>({Dim::X, 2}, {1.1, 2.2});
  auto outD = transform<double, float>(varD, [](const auto x) { return -x; });
  auto outF = transform<double, float>(varF, [](const auto x) { return -x; });
  EXPECT_TRUE(equals(outD.span<double>(), {-1.1, -2.2}));
  EXPECT_TRUE(equals(outF.span<float>(), {-1.1f, -2.2f}));
}

TEST(Variable, apply_binary_in_place) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({}, {3.3});
  transform_in_place<pair_self_t<double>>(
      b, a, [](const auto x, const auto y) { return x + y; });
  EXPECT_TRUE(equals(a.span<double>(), {4.4, 5.5}));
}

TEST(Variable, apply_binary_in_place_var_with_view) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({Dim::Y, 2}, {0.1, 3.3});
  transform_in_place<pair_self_t<double>>(
      b(Dim::Y, 1), a, [](const auto x, const auto y) { return x + y; });
  EXPECT_TRUE(equals(a.span<double>(), {4.4, 5.5}));
}

TEST(Variable, apply_binary_in_place_view_with_var) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({}, {3.3});
  transform_in_place<pair_self_t<double>>(
      b, a(Dim::X, 1), [](const auto x, const auto y) { return x + y; });
  EXPECT_TRUE(equals(a.span<double>(), {1.1, 5.5}));
}

TEST(Variable, apply_binary_in_place_view_with_view) {
  auto a = makeVariable<double>({Dim::X, 2}, {1.1, 2.2});
  const auto b = makeVariable<double>({Dim::Y, 2}, {0.1, 3.3});
  transform_in_place<pair_self_t<double>>(
      b(Dim::Y, 1), a(Dim::X, 1),
      [](const auto x, const auto y) { return x + y; });
  EXPECT_TRUE(equals(a.span<double>(), {1.1, 5.5}));
}

TEST(SparseVariable, create) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  EXPECT_TRUE(var.isSparse());
  EXPECT_EQ(var.sparseDim(), Dim::X);
  // Should we return the full volume here, i.e., accumulate the extents of all
  // the sparse subdata?
  EXPECT_EQ(var.size(), 2);
}

TEST(SparseVariable, dtype) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  // It is not clear that this is the best way of handling things.
  // Variable::dtype() makes sense like this, but it is not so clear for
  // VariableConcept::dtype().
  EXPECT_EQ(var.dtype(), dtype<double>);
  EXPECT_NE(var.data().dtype(), dtype<double>);
}

TEST(SparseVariable, non_sparse_access_fail) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  ASSERT_THROW(var.span<double>(), except::TypeError);
  ASSERT_THROW(var.span<double>(), except::TypeError);
}

TEST(SparseVariable, DISABLED_low_level_access) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  // Need to decide whether we allow this direct access or not.
  ASSERT_THROW((var.span<sparse_container<double>>()), except::TypeError);
}

TEST(SparseVariable, access) {
  const auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  ASSERT_NO_THROW(var.sparseSpan<double>());
  auto data = var.sparseSpan<double>();
  ASSERT_EQ(data.size(), 2);
  EXPECT_TRUE(data[0].empty());
  EXPECT_TRUE(data[1].empty());
}

TEST(SparseVariable, resize_sparse) {
  auto var = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto data = var.sparseSpan<double>();
  data[1] = {1, 2, 3};
}

TEST(SparseVariable, comparison) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};
  auto b = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto b_ = b.sparseSpan<double>();
  b_[0] = {1, 2, 3};
  b_[1] = {1, 2};
  auto c = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto c_ = c.sparseSpan<double>();
  c_[0] = {1, 3};
  c_[1] = {};

  EXPECT_EQ(a, a);
  EXPECT_EQ(a, b);
  EXPECT_EQ(b, a);

  EXPECT_NE(a, c);
  EXPECT_NE(c, a);
}

TEST(SparseVariable, copy) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};

  Variable copy(a);
  EXPECT_EQ(a, copy);
}

TEST(SparseVariable, move) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};

  Variable copy(a);
  Variable moved(std::move(copy));
  EXPECT_EQ(a, moved);
}

TEST(SparseVariable, concatenate) {
  const auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  const auto b = makeSparseVariable<double>({Dim::Y, 3}, Dim::X);
  auto var = concatenate(a, b, Dim::Y);
  EXPECT_TRUE(var.isSparse());
  EXPECT_EQ(var.sparseDim(), Dim::X);
  EXPECT_EQ(var.size(), 5);
}

TEST(SparseVariable, concatenate_along_sparse_dimension) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 2, 3};
  a_[1] = {1, 2};
  auto b = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto b_ = b.sparseSpan<double>();
  b_[0] = {1, 3};
  b_[1] = {};

  auto var = concatenate(a, b, Dim::X);
  EXPECT_TRUE(var.isSparse());
  EXPECT_EQ(var.sparseDim(), Dim::X);
  EXPECT_EQ(var.size(), 2);
  auto data = var.sparseSpan<double>();
  EXPECT_TRUE(equals(data[0], {1, 2, 3, 1, 3}));
  EXPECT_TRUE(equals(data[1], {1, 2}));
}

TEST(SparseVariable, slice) {
  auto var = makeSparseVariable<double>({Dim::Y, 4}, Dim::X);
  auto data = var.sparseSpan<double>();
  data[0] = {1, 2, 3};
  data[1] = {1, 2};
  data[2] = {1};
  data[3] = {};
  auto slice = var(Dim::Y, 1, 3);
  EXPECT_TRUE(slice.isSparse());
  EXPECT_EQ(slice.sparseDim(), Dim::X);
  EXPECT_EQ(slice.size(), 2);
  auto slice_data = slice.sparseSpan<double>();
  EXPECT_TRUE(equals(slice_data[0], {1, 2}));
  EXPECT_TRUE(equals(slice_data[1], {1}));
}

TEST(SparseVariable, unary) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 4, 9};
  a_[1] = {4};

  transform_in_place<sparse_container<double>>(
      a, [](const double x) { return sqrt(x); });
  EXPECT_TRUE(equals(a_[0], {1, 2, 3}));
  EXPECT_TRUE(equals(a_[1], {2}));
}

TEST(SparseVariable, DISABLED_unary_on_sparse_container) {
  auto a = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto a_ = a.sparseSpan<double>();
  a_[0] = {1, 4, 9};
  a_[1] = {4};

  // TODO This is currently broken: The wrong overload of
  // TransformSparse::operator() is selected, so the lambda here is not applied
  // to the whole sparse container (clearing it), but instead to each item of
  // each sparse container. Is there a way to handle this correctly
  // automatically, or do we need to manually specify whether we want to
  // transform items of the variable, or items of the sparse containers that are
  // items of the variable?
  transform_in_place<sparse_container<double>>(
      a, [](const auto &x) { return decltype(x){}; });
  EXPECT_TRUE(a_[0].empty());
  EXPECT_TRUE(a_[1].empty());
}

TEST(SparseVariable, binary_with_dense) {
  auto sparse = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto sparse_ = sparse.sparseSpan<double>();
  sparse_[0] = {1, 2, 3};
  sparse_[1] = {4};
  auto dense = makeVariable<double>({Dim::Y, 2}, {1.5, 0.5});

  transform_in_place<
      pair_custom_t<std::pair<sparse_container<double>, double>>>(
      dense, sparse, [](const double a, const double b) { return a * b; });

  EXPECT_TRUE(equals(sparse_[0], {1.5, 3.0, 4.5}));
  EXPECT_TRUE(equals(sparse_[1], {2.0}));
}

TEST(SparseVariable, operator_plus) {
  auto sparse = makeSparseVariable<double>({Dim::Y, 2}, Dim::X);
  auto sparse_ = sparse.sparseSpan<double>();
  sparse_[0] = {1, 2, 3};
  sparse_[1] = {4};
  auto dense = makeVariable<double>({Dim::Y, 2}, {1.5, 0.5});

  sparse += dense;

  EXPECT_TRUE(equals(sparse_[0], {2.5, 3.5, 4.5}));
  EXPECT_TRUE(equals(sparse_[1], {4.5}));
}
