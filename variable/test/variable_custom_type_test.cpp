// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/string.h"
#include "scipp/variable/variable.tcc"
#include <gtest/gtest.h>

using namespace scipp;
using namespace scipp::variable;

struct CustomType {
  CustomType() {}
  CustomType(const int &) {}
  bool operator==(const CustomType &) const { return true; }
};

// Instantiate Variable type. Test template instantiation with custom type
// argument.
INSTANTIATE_VARIABLE(custom_type, CustomType)

TEST(VariableCustomType, use_custom_templates) {
  auto var = makeVariable<CustomType>(Dimensions{Dim::X, 2},
                                      Values{CustomType{1}, CustomType{2}});
  // Check for bad cast or other built-in implicit type assumptions
  EXPECT_NO_THROW(var.values<CustomType>());
  VariableConstView slice = var.slice(Slice(Dim::X, 0));
  EXPECT_NO_THROW(slice.values<CustomType>());
}

TEST(VariableCustomType, to_string) {
  EXPECT_EQ(to_string(dtype<CustomType>), "custom_type");
}
