// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/string.h"
#include "scipp/dataset/dataset.h"

using namespace scipp;
using namespace scipp::dataset;

TEST(StringTest, null_variable) {
  Variable var;
  ASSERT_FALSE(var);
  ASSERT_NO_THROW(to_string(var));
}

TEST(StringTest, variable_with_dataset_types) {
  ASSERT_NO_THROW(to_string(makeVariable<Dataset>(Dims{}, Shape{})));
  ASSERT_NO_THROW(to_string(makeVariable<DataArray>(
      Values{DataArray(makeVariable<double>(Values{1}))})));
}
