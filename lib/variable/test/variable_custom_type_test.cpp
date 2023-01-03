// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include "scipp/variable/element_array_variable.tcc"
#include "scipp/variable/string.h"
#include <gtest/gtest.h>

using namespace scipp;
using namespace scipp::variable;

struct CustomType {
  CustomType() {}
  CustomType(const int &) {}
  bool operator==(const CustomType &) const { return true; }
};
template <> constexpr DType core::dtype<CustomType>{123456789};

// Instantiate Variable type. Test template instantiation with custom type
// argument.
#define SCIPP_EXPORT
namespace scipp::variable {
INSTANTIATE_ELEMENT_ARRAY_VARIABLE(custom_type, CustomType)
}

TEST(VariableCustomType, use_custom_templates) {
  auto var = makeVariable<CustomType>(Dimensions{Dim::X, 2},
                                      Values{CustomType{1}, CustomType{2}});
  // Check for bad cast or other built-in implicit type assumptions
  EXPECT_NO_THROW(var.values<CustomType>());
  auto slice = var.slice(Slice(Dim::X, 0));
  EXPECT_NO_THROW(slice.values<CustomType>());
}

TEST(VariableCustomType, to_string) {
  EXPECT_EQ(to_string(dtype<CustomType>), "custom_type");
}
