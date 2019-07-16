// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include "scipp/core/dimensions.h"
#include "scipp/core/variable.tcc"
#include <gtest/gtest.h>

using namespace scipp;
using namespace scipp::core;

struct CustomType {
  CustomType() {}
  CustomType(const int &) {}
  bool operator==(const CustomType &) const { return true; }
};

// Instantiate Variable type. Test template instantiation with custom type
// argument.
INSTANTIATE_VARIABLE(CustomType)
// Instantiate VariableConstProxy type. Test template instantiation with
// custom type argument.
INSTANTIATE_SLICEVIEW(CustomType)

TEST(VariableCustomType, use_custom_templates) {
  auto input_values = std::initializer_list<CustomType>{1, 2};
  auto var = Variable{{Dim::X, 2}, input_values};
  // Check for bad cast or other built-in implicit type assumptions
  EXPECT_NO_THROW(var.values<CustomType>());
  VariableConstProxy slice = var.slice(Slice(Dim::X, 0));
  EXPECT_NO_THROW(slice.values<CustomType>());
}
