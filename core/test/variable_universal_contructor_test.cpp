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
TEST(VariableUniversalConstructorTest, universal_make_variable) {
  auto variable = Variable(dtype<float>, Dimensions{{Dim::X, Dim::Y}, {2, 3}},
                           units::Unit(units::kg));

  EXPECT_EQ(variable.dims(), (Dimensions{{Dim::X, Dim::Y}, {2, 3}}));
  EXPECT_EQ(variable.unit(), units::kg);
  EXPECT_EQ(variable.values<float>().size(), 6);
  EXPECT_FALSE(variable.hasVariances());

  auto otherVariable =
      Variable(dtype<float>, Dimensions{{Dim::X, Dim::Y}, {2, 3}});
  variable.setUnit(units::dimensionless);

  EXPECT_EQ(variable, otherVariable);
  auto data = Vector<double>{1.0, 4.5, 2.7, 5.0, 7.0, 6.7};

  {
    auto val = Values<double>(Vector<double>(data));
    auto valAddr = &(*val.data)[0];
    auto var = Variances<double>(std::optional(Vector<double>(data)));
    auto varAddr = &(*var.data)[0];

    variable = Variable(dtype<double>, Dimensions{{Dim::X, Dim::Y}, {2, 3}},
                        std::move(val), units::Unit(units::kg), std::move(var));

    auto vval = variable.values<double>();
    auto vvar = variable.variances<double>();
    EXPECT_TRUE(equals(vval, data));
    EXPECT_TRUE(equals(vvar, data));
    EXPECT_EQ(&vval[0], valAddr);
    EXPECT_EQ(&vvar[0], varAddr);
  }

  {
    auto val = Values<double>(Vector<double>(data));
    auto var = Variances<double>(std::optional(Vector<double>(data)));
    variable = Variable(dtype<int64_t>, Dimensions{{Dim::X, Dim::Y}, {2, 3}},
                        std::move(val), units::Unit(units::kg), std::move(var));

    EXPECT_EQ(variable.dtype(), dtype<int64_t>);
    EXPECT_TRUE(equals(variable.values<int64_t>(),
                       Vector<int64_t>(data.begin(), data.end())));
    EXPECT_TRUE(equals(variable.variances<int64_t>(),
                       Vector<int64_t>(data.begin(), data.end())));
  }
}
