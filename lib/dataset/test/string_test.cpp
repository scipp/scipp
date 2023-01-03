// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(StringTest, null_variable) {
  Variable var;
  ASSERT_FALSE(var.is_valid());
  ASSERT_NO_THROW(to_string(var));
}

TEST(StringTest, variable_with_dataset_types) {
  ASSERT_NO_THROW(to_string(makeVariable<Dataset>(Dims{}, Shape{})));
  ASSERT_NO_THROW(to_string(makeVariable<DataArray>(
      Values{DataArray(makeVariable<double>(Values{1}))})));
}
