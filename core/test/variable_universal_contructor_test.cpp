// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/core/dataset.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/except.h"
#include "scipp/core/variable.h"
#include "scipp/core/variable.tcc"

using namespace scipp;
using namespace scipp::core;

// TEST(VariableUniversalConstructorTest, dimensions_unit_basic) {
//  auto variable = Variable(dtype<float>, Dimensions{{Dim::X, Dim::Y}, {2, 3}},
//                           units::Unit(units::kg));
//
//  EXPECT_EQ(variable.dims(), (Dimensions{{Dim::X, Dim::Y}, {2, 3}}));
//  EXPECT_EQ(variable.unit(), units::kg);
//  EXPECT_EQ(variable.values<float>().size(), 6);
//  EXPECT_FALSE(variable.hasVariances());
//
//  auto otherVariable =
//      Variable(dtype<float>, Dimensions{{Dim::X, Dim::Y}, {2, 3}});
//  variable.setUnit(units::dimensionless);
//  EXPECT_EQ(variable, otherVariable);
//
//  auto oneMore = Variable(dtype<float>, units::Unit(units::dimensionless),
//                          Dimensions{{Dim::X, Dim::Y}, {2, 3}});
//  EXPECT_EQ(oneMore, variable);
//}

TEST(VariableUniversalConstructorTest, type_test) {
  auto flt = std::vector{1.5f, 3.6f};
  Variable(dtype<Eigen::Vector3d>, Dimensions{{Dim::X, Dim::Y}, {2, 1}},
           Values(flt.begin(), flt.end()));
}

TEST(VariableUniversalConstructorTest, type_construcors_mix) {
  auto flt = std::vector{1.5f, 3.6f};
  auto v1 =
      Variable(dtype<float>, Dimensions{{Dim::X, Dim::Y}, {2, 1}},
               Values(flt.begin(), flt.end()) /*, Variances({2.0, 3.0})*/);
  //  auto v2 = Variable(dtype<float>, Dimensions{{Dim::X, Dim::Y}, {2, 1}},
  //                     Values({1.5, 3.6}), Variances({2, 3}));
  //  auto v3 = Variable(dtype<float>, units::Unit(),
  //                     Dimensions{{Dim::X, Dim::Y}, {2, 1}},
  //                     Values<float>({1.5f, 3.6}));
  //  v3.setVariances(Vector<float>{2, 3});
  //  EXPECT_EQ(v1, v2);
  //  EXPECT_EQ(v1, v3);
  //
  //  v2 = Variable(dtype<float>, Variances({2.0, 3.0}),
  //                Dimensions{{Dim::X, Dim::Y}, {2, 1}}, Values({1.5f, 3.6f}));
  //  EXPECT_EQ(v1, v2);
}

// TEST(VariableUniversalConstructorTest, no_copy_on_matched_types) {
//  auto data = Vector<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};
//  auto val = Values(Vector<double>(data));
//  auto valAddr = std::get<0>(val.tuple).data();
//  auto var = Variances(Vector<double>(data));
//  auto varAddr = std::get<0>(var.tuple).data();
//
//  auto variable =
//      Variable(dtype<double>, Dimensions{{Dim::X, Dim::Y}, {2, 3}},
//               std::move(val), units::Unit(units::kg), std::move(var));
//
//  auto vval = variable.values<double>();
//  auto vvar = variable.variances<double>();
//  EXPECT_TRUE(equals(vval, data));
//  EXPECT_TRUE(equals(vvar, data));
//  EXPECT_EQ(&vval[0], valAddr);
//  EXPECT_EQ(&vvar[0], varAddr);
//}

// TEST(VariableUniversalConstructorTest, convertable_types) {
//  auto data = Vector<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};
//  auto val = Values(Vector<double>(data));
//  auto var = Variances(Vector<double>(data));
//  auto variable =
//      Variable(dtype<int64_t>, Dimensions{{Dim::X, Dim::Y}, {2, 3}},
//               std::move(val), units::Unit(units::kg), std::move(var));
//
//  EXPECT_EQ(variable.dtype(), dtype<int64_t>);
//  EXPECT_TRUE(equals(variable.values<int64_t>(),
//                     Vector<int64_t>(data.begin(), data.end())));
//  EXPECT_TRUE(equals(variable.variances<int64_t>(),
//                     Vector<int64_t>(data.begin(), data.end())));
//}

// TEST(VariableUniversalConstructorTest, unconvertable_types) {
//  EXPECT_THROW(Variable(dtype<Eigen::Vector3d>,
//                        Dimensions{{Dim::X, Dim::Y}, {2, 1}},
//                        Values({1.5f, 3.6f}), Variances({2.0, 3.0})),
//               except::TypeError);
//}
