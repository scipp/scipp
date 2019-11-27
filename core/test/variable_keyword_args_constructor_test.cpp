// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;
TEST(CreateVariableTest, construct_sparse) {
  auto var = createVariable<double>(Dims{Dim::X, Dim::Y},
                                    Shape{2, Dimensions::Sparse});

  auto dims = Dimensions();
  createVariable<int64_t>(Dims(dims.labels()), Shape(dims.shape()));
}

TEST(VariableUniversalConstructorTest, dimensions_unit_basic) {
  auto variable = Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 3},
                           units::Unit(units::kg));

  EXPECT_EQ(variable.dims(), (Dimensions{{Dim::X, Dim::Y}, {2, 3}}));
  EXPECT_EQ(variable.unit(), units::kg);
  EXPECT_EQ(variable.values<float>().size(), 6);
  EXPECT_FALSE(variable.hasVariances());

  auto otherVariable =
      Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 3});
  variable.setUnit(units::dimensionless);
  EXPECT_EQ(variable, otherVariable);

  auto oneMore = Variable(dtype<float>, units::Unit(units::dimensionless),
                          Dims{Dim::X, Dim::Y}, Shape{2, 3});
  EXPECT_EQ(oneMore, variable);
}

TEST(VariableUniversalConstructorTest, type_construcors_mix) {
  auto flt = std::vector{1.5f, 3.6f};
  auto v1 = Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 1},
                     Values(flt.begin(), flt.end()),
                     Variances(Vector<double>{2.0, 3.0}));
  auto v2 =
      Variable(dtype<float>, Dims{Dim::X, Dim::Y}, Shape{2, 1},
               Values(Vector<double>{1.5, 3.6}), Variances(Vector<int>{2, 3}));
  auto v3 = Variable(dtype<float>, units::Unit(), Dims{Dim::X, Dim::Y},
                     Shape{2, 1}, Values(Vector<double>{1.5f, 3.6}));
  v3.setVariances(Vector<float>{2, 3});
  EXPECT_EQ(v1, v2);
  EXPECT_EQ(v1, v3);

  v2 = Variable(dtype<float>, Variances(Vector<double>{2.0, 3.0}),
                Dims{Dim::X, Dim::Y}, Shape{2, 1},
                Values(Vector<float>{1.5f, 3.6f}));
  EXPECT_EQ(v1, v2);
}

TEST(VariableUniversalConstructorTest, no_copy_on_matched_types) {
  auto values = Vector<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};
  auto variances = Vector<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};
  auto valuesRef = Vector<double>(values);
  auto variancesRef = Vector<double>(variances);
  auto valAddr = values.data();
  auto varAddr = variances.data();

  auto variable = Variable(dtype<double>, Dims{Dim::X, Dim::Y}, Shape{2, 3},
                           Values(std::move(values)), units::Unit(units::kg),
                           Variances(std::move(variances)));

  auto vval = variable.values<double>();
  auto vvar = variable.variances<double>();
  EXPECT_TRUE(equals(vval, valuesRef));
  EXPECT_TRUE(equals(vvar, variancesRef));
  EXPECT_EQ(&vval[0], valAddr);
  EXPECT_EQ(&vvar[0], varAddr);
}

TEST(VariableUniversalConstructorTest, convertable_types) {
  auto data = Vector<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};
  auto variable = Variable(dtype<int64_t>, Dims{Dim::X, Dim::Y}, Shape{2, 3},
                           Values(Vector<double>(data)), units::Unit(units::kg),
                           Variances(Vector<double>(data)));

  EXPECT_EQ(variable.dtype(), dtype<int64_t>);
  EXPECT_TRUE(equals(variable.values<int64_t>(),
                     Vector<int64_t>(data.begin(), data.end())));
  EXPECT_TRUE(equals(variable.variances<int64_t>(),
                     Vector<int64_t>(data.begin(), data.end())));
}

TEST(VariableUniversalConstructorTest, unconvertable_types) {
  EXPECT_THROW(Variable(dtype<Eigen::Vector3d>, Dims{Dim::X, Dim::Y},
                        Shape{2, 1}, Values{1.5f, 3.6f}, Variances{2.0, 3.0}),
               except::TypeError);
}

TEST(VariableUniversalConstructorTest, initializer_list) {
  EXPECT_EQ(Variable(dtype<int32_t>, Dims{Dim::X}, Shape{2}, Values{1.0, 1.0}),
            Variable(dtype<int32_t>, Dims{Dim::X}, Shape{2},
                     Values(Vector<int32_t>(2, 1))));
  EXPECT_EQ(Variable(dtype<int32_t>, Values{1.0, 1.0}, Dims{Dim::X}, Shape{2},
                     Variances{2.0f, 2.0f}),
            Variable(dtype<int32_t>, Dims{Dim::X}, Shape{2},
                     Values(Vector<int32_t>(2, 1)),
                     Variances(Vector<double>(2, 2))));
}
