// SPDX-License-Identifier: BSD-3-Clause Copyright (c) 2021 Scipp contributors
// (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"

using namespace scipp;

template <class T> void check_equal(const T &var) {
  EXPECT_TRUE(equals_nan(var, var));
  EXPECT_TRUE(equals_nan(var, copy(var)));
  EXPECT_NE(var, var);
  EXPECT_NE(var, copy(var));
}

class EqualsNanTest : public ::testing::Test {
protected:
  EqualsNanTest() { ds.setData("a", da); }
  Dimensions dims{Dim::Y, 2};
  Variable indices = makeVariable<scipp::index_pair>(
      dims, Values{std::pair{0, 2}, std::pair{2, 4}});
  Variable data =
      makeVariable<double>(Dims{Dim::X}, Shape{4}, Values{1, 2, 3, 4});
  DataArray da =
      DataArray(data, {{Dim::X, data + data}}, {{"mask", data + data}},
                {{Dim("attr"), data + data}});
  Variable nan = makeVariable<double>(Dims{Dim::X}, Shape{4},
                                      Values{1.0f, 2.0f, NAN, 4.0f});
  Dataset ds;

  void check() {
    check_equal(da);
    check_equal(ds);
    check_equal(makeVariable<DataArray>(Values{da}));
    check_equal(makeVariable<Dataset>(Values{ds}));
    check_equal(make_bins(indices, Dim::X, da));
    check_equal(make_bins(indices, Dim::X, ds));
  }
};

TEST_F(EqualsNanTest, nan_data) {
  da += nan;
  check();
}

TEST_F(EqualsNanTest, nan_coord) {
  da.coords()[Dim::X] += nan;
  check();
}

TEST_F(EqualsNanTest, nan_mask) {
  da.masks()["mask"] += nan;
  check();
}

TEST_F(EqualsNanTest, nan_attr) {
  da.attrs()[Dim("attr")] += nan;
  check();
}
