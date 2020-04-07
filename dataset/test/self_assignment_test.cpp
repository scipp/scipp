// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
//
// The test in this file ensure that comparison operators for Dataset and
// DatasetConstView are correct. More complex tests should build on the
// assumption that comparison operators are correct.
#include "test_macros.h"
#include <gtest/gtest.h>

#include <numeric>

#include "dataset_test_common.h"
#include "scipp/core/dimensions.h"
#include "scipp/dataset/dataset.h"

using namespace scipp;
using namespace scipp::dataset;

class SelfAssignmentTest : public ::testing::Test {
protected:
  SelfAssignmentTest() {
    dataset.setData("a",
                    makeVariable<double>(Dims{Dim::X}, Shape{2}, Values{1, 2}));
  }

  Dataset dataset;
};

TEST_F(SelfAssignmentTest, dataset_item) {
  const DataArray expected(dataset["a"]);
  const auto *expected_ptr = dataset["a"].values<double>().data();
  dataset.setData("a", dataset["a"]);
  EXPECT_EQ(dataset["a"], expected);
  EXPECT_EQ(dataset["a"].values<double>().data(), expected_ptr);

  // Code that checks for self-assignment might erroneously not check for
  // presence of slices.
  dataset.setData("a", dataset["a"].slice({Dim::X, 0, 1}));
  EXPECT_NE(dataset["a"], expected);
  EXPECT_NE(dataset["a"].values<double>().data(), expected_ptr);
}

TEST_F(SelfAssignmentTest, data_view_assign) {
  const DataArray expected(dataset["a"]);
  const auto *expected_ptr = dataset["a"].values<double>().data();
  dataset["a"].assign(dataset["a"]);
  EXPECT_EQ(dataset["a"], expected);
  EXPECT_EQ(dataset["a"].values<double>().data(), expected_ptr);

  // Code that checks for self-assignment might erroneously not check for
  // presence of slices.
  dataset["a"].slice({Dim::X, 0, 1}).assign(dataset["a"].slice({Dim::X, 1, 2}));
  EXPECT_NE(dataset["a"], expected);
  EXPECT_EQ(dataset["a"].values<double>().data(), expected_ptr);
}

TEST_F(SelfAssignmentTest, variable_view_assign) {
  const Variable expected(dataset["a"].data());
  const auto *expected_ptr = dataset["a"].values<double>().data();

  // Without slices the view just forward to the data in the underlying
  // variable, so we test 2 cases here, without and with slice.
  dataset["a"].data().assign(dataset["a"].data());
  EXPECT_EQ(dataset["a"].data(), expected);
  EXPECT_EQ(dataset["a"].values<double>().data(), expected_ptr);

  dataset["a"]
      .data()
      .slice({Dim::X, 0, 1})
      .assign(dataset["a"].data().slice({Dim::X, 0, 1}));
  // There is no reasonable way to test that no actual copy has happened, this
  // would pass even if the self-assignment would actually assign all the
  // elements.
  EXPECT_EQ(dataset["a"].data(), expected);
  EXPECT_EQ(dataset["a"].values<double>().data(), expected_ptr);
}
