// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/dataset.h"
#include "scipp/variable/arithmetic.h"

#include "dataset_test_common.h"

using namespace scipp;
using namespace scipp::dataset;

struct CopyTest : public ::testing::Test {
  CopyTest() : dataset(factory.make("data")), array(copy(dataset["data"])) {}

protected:
  DatasetFactory factory;
  Dataset dataset;
  DataArray array;
};
TEST_F(CopyTest, data_array) { EXPECT_EQ(copy(array), array); }
TEST_F(CopyTest, dataset) { EXPECT_EQ(copy(dataset), dataset); }
DataArray make_dataarray_with_bin_edges() {
  return DataArray(makeVariable<double>(Dimensions{Dim::X, 2}, Values({0, 1})),
                   {{Dim::X, makeVariable<double>(Dimensions{Dim::X, 3},
                                                  Values({0, 1, 2}))}});
}
TEST_F(CopyTest, dataarray_with_bin_edge_coord) {
  auto a = make_dataarray_with_bin_edges();
  auto b = copy(a);
  EXPECT_EQ(a, b);
}
TEST_F(CopyTest, dataset_with_bin_edge_coord) {
  auto a = Dataset{make_dataarray_with_bin_edges()};
  auto b = copy(a);
  EXPECT_EQ(a, b);
  for (const auto &[dim, item] : b.coords()) {
    EXPECT_NE(a.coords().at(dim).values<double>().data(),
              item.values<double>().data());
  }
}

struct CopyOutArgTest : public CopyTest {
  CopyOutArgTest() : dataset_copy(copy(dataset)), array_copy(copy(array)) {
    const auto one = 1.0 * sc_units::one;
    array_copy.data() += one;
    array_copy.coords()[Dim::X] += one;
    array_copy.coords()[Dim::Y] += one;
    copy(~array_copy.masks()["mask"], array_copy.masks()["mask"]);
    EXPECT_NE(array_copy, array);
    dataset_copy["data"].data() += one;
    dataset_copy.coords()[Dim::X] += one;
    dataset_copy.coords()[Dim::Y] += one;
    copy(~array_copy.masks()["mask"], dataset_copy["data"].masks()["mask"]);
    EXPECT_NE(dataset_copy, dataset);
  }

protected:
  Dataset dataset_copy;
  DataArray array_copy;
};

TEST_F(CopyOutArgTest, data_array_out_arg) {
  // copy with out arg also copies coords and masks
  EXPECT_EQ(copy(array, array_copy), array);
  EXPECT_EQ(array_copy, array);
}

TEST_F(CopyOutArgTest, dataset_out_arg) {
  // copy with out arg also copies coords and masks
  EXPECT_EQ(copy(dataset, dataset_copy), dataset);
  EXPECT_EQ(dataset_copy, dataset);
}
