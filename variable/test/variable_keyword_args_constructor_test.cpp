// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/core/except.h"
#include "scipp/variable/variable.h"

using namespace scipp;
using namespace scipp::variable;

TEST(CreateVariableTest, from_single_value) {
  auto var = makeVariable<float>(Values{0}, Variances{1});
  EXPECT_EQ(var.dtype(), dtype<float>);
  EXPECT_EQ(var.value<float>(), 0.0f);
  EXPECT_EQ(var.variance<float>(), 1.0f);
}

TEST(CreateVariableTest, dims_shape) {
  // Check that we never use std::vector::vector(size, value)
  EXPECT_EQ(makeVariable<float>(Dims{Dim::X}, Shape{2}),
            makeVariable<float>(Dimensions({{Dim::X, 2}})));
  EXPECT_EQ(makeVariable<float>(Dims{Dim::X}, Shape{2l}),
            makeVariable<float>(Dimensions({{Dim::X, 2}})));
  EXPECT_EQ(makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape{2l, 3}),
            makeVariable<float>(Dimensions({{Dim::X, 2}, {Dim::Y, 3}})));
  EXPECT_EQ(makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape{2l, 3l}),
            makeVariable<float>(Dimensions({{Dim::X, 2}, {Dim::Y, 3}})));
  EXPECT_EQ(makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape{2, 3}),
            makeVariable<float>(Dimensions({{Dim::X, 2}, {Dim::Y, 3}})));
  EXPECT_EQ(makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape(2, 3)),
            makeVariable<float>(Dimensions({{Dim::X, 2}, {Dim::Y, 3}})));
  EXPECT_EQ(makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape(2l, 3)),
            makeVariable<float>(Dimensions({{Dim::X, 2}, {Dim::Y, 3}})));
}

TEST(CreateVariableTest, dims_shape_order) {
  EXPECT_EQ(makeVariable<float>(Dims{Dim::X}, Shape{2}),
            makeVariable<float>(Shape{2}, Dims{Dim::X}));
}

TEST(CreateVariableTest, default_init) {
  auto noVariance = makeVariable<float>(Dims{Dim::X}, Shape{3});
  auto stillNoVariance = makeVariable<float>(Dims{Dim::X}, Shape{3}, Values{});
  auto withVariance =
      makeVariable<float>(Dims{Dim::X}, Shape{3}, Values{}, Variances{});

  EXPECT_FALSE(noVariance.hasVariances());
  EXPECT_FALSE(stillNoVariance.hasVariances());
  EXPECT_TRUE(withVariance.hasVariances());
  EXPECT_EQ(noVariance.values<float>().size(), 3);
  EXPECT_EQ(stillNoVariance.values<float>().size(), 3);
  EXPECT_EQ(withVariance.values<float>().size(), 3);
  EXPECT_EQ(withVariance.variances<float>().size(), 3);

  EXPECT_ANY_THROW(makeVariable<float>(Values(0), Variances(0)));

  auto otherWithVariance = makeVariable<float>(Values{}, Variances{});
  EXPECT_EQ(otherWithVariance.values<float>().size(), 1);
  EXPECT_EQ(otherWithVariance.variances<float>().size(), 1);
}

TEST(CreateVariableTest, from_vector) {
  EXPECT_EQ(makeVariable<double>(Dims{Dim::X}, Shape{3},
                                 Values(std::vector<int>{1, 2, 3})),
            makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));

  const std::vector<double> v{1, 2, 3};
  auto varRef = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3});
  auto var = makeVariable<double>(Dims{Dim::X}, Shape{3}, Values(v));
}

TEST(CreateVariableTest, construct_events) {
  auto var = makeVariable<event_list<double>>(Dims{Dim::X}, Shape{2});

  auto dimensions = Dimensions{Dim::X, 2};
  makeVariable<event_list<double>>(
      Dimensions{dimensions},
      Values{event_list<double>(), event_list<double>()},
      Variances{event_list<double>(), event_list<double>()});
}

TEST(VariableUniversalConstructorTest, dimensions_unit_basic) {
  auto variable =
      Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 3}, units::kg);

  EXPECT_EQ(variable.dims(), (Dimensions{{Dim::X, Dim::Y}, {2, 3}}));
  EXPECT_EQ(variable.unit(), units::kg);
  EXPECT_EQ(variable.values<float>().size(), 6);
  EXPECT_FALSE(variable.hasVariances());

  auto otherVariable =
      Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 3});
  variable.setUnit(units::one);
  EXPECT_EQ(variable, otherVariable);

  auto oneMore =
      Variable(dtype<float>, units::one, Dims{Dim::X, Dim::Y}, Shape{2, 3});
  EXPECT_EQ(oneMore, variable);
}

TEST(VariableUniversalConstructorTest, type_constructors_mix) {
  auto flt = std::vector{1.5f, 3.6f};
  auto v1 = Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 1},
                     Values(flt.begin(), flt.end()), Variances{2.0, 3.0});
  auto v2 = Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 1},
                     Values{1.5, 3.6}, Variances{2, 3});
  auto v3 = Variable(dtype<float>, units::Unit(), Dims{Dim::X, Dim::Y},
                     Shape{2, 1}, Values{1.5f, 3.6f});
  v3.setVariances(
      makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape{2, 1}, Values{2, 3}));
  EXPECT_EQ(v1, v2);
  EXPECT_EQ(v1, v3);

  v2 = Variable(dtype<float>, Variances{2.0, 3.0}, Dims{Dim::X, Dim::Y},
                Shape{2, 1}, Values{1.5f, 3.6f});
  EXPECT_EQ(v1, v2);
}

TEST(VariableUniversalConstructorTest, no_copy_on_matched_types) {
  using namespace scipp::variable::detail;
  auto values = element_array<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};
  auto variances = element_array<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};
  auto valuesRef = element_array<double>(values);
  auto variancesRef = element_array<double>(variances);
  auto valAddr = values.data();
  auto varAddr = variances.data();

  auto variable = Variable(dtype<double>, Dims{Dim::X, Dim::Y}, Shape{2, 3},
                           Values(std::move(values)), units::kg,
                           Variances(std::move(variances)));

  auto vval = variable.values<double>();
  auto vvar = variable.variances<double>();
  EXPECT_TRUE(equals(vval, valuesRef));
  EXPECT_TRUE(equals(vvar, variancesRef));
  EXPECT_EQ(&vval[0], valAddr);
  EXPECT_EQ(&vvar[0], varAddr);
}

TEST(VariableUniversalConstructorTest, convertable_types) {
  using namespace scipp::variable::detail;
  auto data = std::vector<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};
  auto variable = Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 3},
                           Values(data), units::kg, Variances(data));

  EXPECT_EQ(variable.dtype(), dtype<float>);
  EXPECT_TRUE(equals(variable.values<float>(),
                     std::vector<float>(data.begin(), data.end())));
  EXPECT_TRUE(equals(variable.variances<float>(),
                     std::vector<float>(data.begin(), data.end())));
}

TEST(VariableUniversalConstructorTest, unconvertable_types) {
  EXPECT_THROW(Variable(dtype<Eigen::Vector3d>, Dims{Dim::X, Dim::Y},
                        Shape{2, 1}, Values{1.5f, 3.6f}, Variances{2.0, 3.0}),
               except::TypeError);
}

TEST(VariableUniversalConstructorTest, initializer_list) {
  EXPECT_EQ(Variable(dtype<int32_t>, Dims{Dim::X}, Shape{2}, Values{1.0, 1.0}),
            Variable(dtype<int32_t>, Dims{Dim::X}, Shape{2},
                     Values(std::vector<int32_t>(2, 1))));
  EXPECT_EQ(Variable(dtype<float>, Values{1.0, 1.0}, Dims{Dim::X}, Shape{2},
                     Variances{2.0f, 2.0f}),
            Variable(dtype<float>, Dims{Dim::X}, Shape{2},
                     Values(std::vector<int32_t>(2, 1)),
                     Variances(std::vector<double>(2, 2))));
}

TEST(VariableUniversalConstructorTest, from_vector) {
  EXPECT_EQ(makeVariable<double>(Dims{Dim::X}, Shape{3},
                                 Values(std::vector<int>{1, 2, 3})),
            makeVariable<double>(Dims{Dim::X}, Shape{3}, Values{1, 2, 3}));
}
