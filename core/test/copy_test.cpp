// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

struct CopyTest : public ::testing::Test {
  CopyTest() : dataset(factory.make()), array(dataset["data_xyz"]) {}

protected:
  DatasetFactory3D factory;
  Dataset dataset;
  DataArrayView array;
};

TEST_F(CopyTest, data_array) { EXPECT_EQ(copy(array), array); }

TEST_F(CopyTest, data_array_drop_attrs) {
  const auto attr = makeVariable<double>(Values{1});
  array.attrs().set("attr", attr);

  auto copied = copy(array, AttrPolicy::Drop);

  EXPECT_NE(copied, array);
  copied.attrs().set("attr", attr);
  EXPECT_EQ(copied, array);
}
