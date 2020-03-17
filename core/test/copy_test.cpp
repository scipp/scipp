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
TEST_F(CopyTest, dataset) { EXPECT_EQ(copy(dataset), dataset); }

TEST_F(CopyTest, data_array_drop_attrs) {
  auto copied = copy(array, AttrPolicy::Drop);

  EXPECT_NE(copied, array);
  copied.attrs().set("attr", attr);
  EXPECT_EQ(copied, array);
}

TEST_F(CopyTest, dataset_drop_attrs) {
  // not implemented yet
  EXPECT_ANY_THROW(copy(dataset, AttrPolicy::Drop));
}

struct CopyOutArgTest : public CopyTest {
  CopyOutArgTest() : dataset_copy(copy(dataset)), array_copy(copy(array)) {
    array_copy.data() += 1.0;
    array_copy.coords()[Dim::X] += 1.0;
    array_copy.coords()[Dim::Y] += 1.0;
    array_copy.masks()["masks_x"].assign(~array_copy.masks()["masks_x"]);
    array_copy.attrs()["attr"] += 1.0;
    EXPECT_NE(array_copy, array);
    dataset_copy["data_xyz"].data() += 1.0;
    dataset_copy["data_xyz"].attrs()["attr"] += 1.0;
    dataset_copy.coords()[Dim::X] += 1.0;
    dataset_copy.coords()[Dim::Y] += 1.0;
    dataset_copy.masks()["masks_x"].assign(~array_copy.masks()["masks_x"]);
    dataset_copy.attrs()["attr_x"] += 1.0;
    EXPECT_NE(dataset_copy, dataset);
  }

protected:
  Dataset dataset_copy;
  DataArray array_copy;
};

TEST_F(CopyOutArgTest, data_array_out_arg) {
  // copy with out arg also copies coords, masks, and attrs
  EXPECT_EQ(copy(array, array_copy), array);
  EXPECT_EQ(array_copy, array);
}

TEST_F(CopyOutArgTest, dataset_out_arg) {
  // copy with out arg also copies coords, masks, and attrs
  EXPECT_EQ(copy(dataset, dataset_copy), dataset);
  EXPECT_EQ(dataset_copy, dataset);
}

TEST_F(CopyOutArgTest, data_array_out_arg_drop_attrs) {
  array_copy.attrs()["attr"].assign(array.attrs()["attr"]);

  // copy with out arg also copies coords, masks, and attrs
  EXPECT_EQ(copy(array, array_copy, AttrPolicy::Drop), array);
  EXPECT_EQ(array_copy, array);
}

TEST_F(CopyOutArgTest, dataset_out_arg_drop_attrs) {
  dataset_copy.attrs()["attr_x"].assign(dataset.attrs()["attr_x"]);
  dataset_copy["data_xyz"].attrs()["attr"].assign(
      dataset["data_xyz"].attrs()["attr"]);

  // copy with out arg also copies coords, masks, and attrs
  EXPECT_EQ(copy(dataset, dataset_copy, AttrPolicy::Drop), dataset);
  EXPECT_EQ(dataset_copy, dataset);
}

TEST_F(CopyOutArgTest, data_array_out_arg_drop_attrs_untouched) {
  // copy with out arg leaves items in output that are not in the input
  // untouched. This also applies to dropped attributes.
  EXPECT_NE(copy(array, array_copy, AttrPolicy::Drop), array);
  EXPECT_NE(array_copy, array);
  array_copy.attrs()["attr"].assign(array.attrs()["attr"]);
  EXPECT_EQ(array_copy, array);
}

TEST_F(CopyOutArgTest, dataset_out_arg_drop_attrs_untouched) {
  // copy with out arg leaves items in output that are not in the input
  // untouched. This also applies to dropped attributes.
  EXPECT_NE(copy(dataset, dataset_copy, AttrPolicy::Drop), dataset);
  EXPECT_NE(dataset_copy, dataset);
  dataset_copy.attrs()["attr_x"].assign(dataset.attrs()["attr_x"]);
  dataset_copy["data_xyz"].attrs()["attr"].assign(
      dataset["data_xyz"].attrs()["attr"]);
  EXPECT_EQ(dataset_copy, dataset);
}
