// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::core;

struct CopyTest : public ::testing::Test {
  CopyTest() : dataset(factory.make()), array(dataset["data_xyz"]) {
    array.attrs().set("attr", attr);
  }

protected:
  DatasetFactory3D factory;
  Dataset dataset;
  DataArrayView array;
  Variable attr = makeVariable<double>(Values{1});
};

TEST_F(CopyTest, data_array) { EXPECT_EQ(copy(array), array); }

TEST_F(CopyTest, data_array_drop_attrs) {
  auto copied = copy(array, AttrPolicy::Drop);

  EXPECT_NE(copied, array);
  copied.attrs().set("attr", attr);
  EXPECT_EQ(copied, array);
}

struct CopyOutArgTest : public CopyTest {
  CopyOutArgTest() : copied(copy(array)) {
    copied.data() += 1.0;
    copied.coords()[Dim::X] += 1.0;
    copied.coords()[Dim::Y] += 1.0;
    copied.masks()["masks_x"].assign(~copied.masks()["masks_x"]);
    copied.attrs()["attr"] += 1.0;
    EXPECT_NE(copied, array);
  }

protected:
  DataArray copied;
};

TEST_F(CopyOutArgTest, data_array_out_arg) {
  // copy with out arg also copies coords, masks, and attrs
  EXPECT_EQ(copy(array, copied), array);
  EXPECT_EQ(copied, array);
}

TEST_F(CopyOutArgTest, data_array_out_arg_drop_attrs) {
  copied.attrs()["attr"].assign(array.attrs()["attr"]);

  // copy with out arg also copies coords, masks, and attrs
  EXPECT_EQ(copy(array, copied, AttrPolicy::Drop), array);
  EXPECT_EQ(copied, array);
}

TEST_F(CopyOutArgTest, data_array_out_arg_drop_attrs_untouched) {
  // copy with out arg leaves items in output that are not in the input
  // untouched. This also applies to dropped attributes.
  EXPECT_NE(copy(array, copied, AttrPolicy::Drop), array);
  EXPECT_NE(copied, array);
  copied.attrs()["attr"].assign(array.attrs()["attr"]);
  EXPECT_EQ(copied, array);
}
