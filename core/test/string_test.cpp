// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/string.h"
#include "scipp/core/variable.h"

using namespace scipp;
using namespace scipp::core;

TEST(StringTest, null_variable) {
  Variable var;
  ASSERT_FALSE(var);
  ASSERT_NO_THROW(to_string(var));
}
