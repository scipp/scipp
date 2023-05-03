// SPDX-License-Identifier: BSD-3-Clause Copyright (c) 2022 Scipp contributors
// (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/bins.h"
#include "scipp/dataset/shape.h"

#include "test_macros.h"

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
    da.masks().erase("mask");
    ds["a"].masks().erase("mask");
    ASSERT_NO_THROW_DISCARD(da + da);
    ASSERT_NO_THROW_DISCARD(da + copy(da));
    ASSERT_NO_THROW_DISCARD(ds + ds);
    ASSERT_NO_THROW_DISCARD(ds + copy(ds));
    ASSERT_NO_THROW_DISCARD(ds.setData("b", da));
    ASSERT_NO_THROW_DISCARD(ds.setData("b", copy(da)));
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
  const auto out = da + copy(da);
  ASSERT_TRUE(out.attrs().contains(Dim("attr")));
}

TEST_F(EqualsNanTest, concat_nan_coord) {
  da.coords()[Dim::X] += nan;
  const auto out = dataset::concat(std::vector{da, copy(da)}, Dim::Y);
  ASSERT_TRUE(equals_nan(out.coords()[Dim::X], da.coords()[Dim::X]));
}

TEST_F(EqualsNanTest, concat_nan_mask) {
  da.masks()["mask"] += nan;
  const auto out = dataset::concat(std::vector{da, copy(da)}, Dim::Y);
  ASSERT_TRUE(equals_nan(out.masks()["mask"], da.masks()["mask"]));
}

TEST_F(EqualsNanTest, concat_nan_attr) {
  da.attrs()[Dim("attr")] += nan;
  const auto out = dataset::concat(std::vector{da, copy(da)}, Dim::Y);
  ASSERT_TRUE(equals_nan(out.attrs()[Dim("attr")], da.attrs()[Dim("attr")]));
}

TEST_F(EqualsNanTest, concat_nan_item) {
  da.masks().erase("mask");
  ds["a"].masks().erase("mask");
  ds.setData("b", copy(da));
  ds["a"] += nan;
  auto ds2 = copy(ds);
  ds2["b"] += da;
  const auto out = dataset::concat(std::vector{ds, ds2}, Dim::Y);
  // concat will avoid broadcasting "a" since it is equal, need to accepts NAN
  ASSERT_TRUE(equals_nan(out["a"], da));
}

TEST_F(EqualsNanTest, dataset_item_self_assign) {
  // This is relevant for d['x', 1:]['a'] *= 1.5. We should accept NaNs. Current
  // implementation in Dataset.setData does not, because pointer comparison
  // kicks in first, so this just works.
  auto item = ds.slice({Dim::X, 0})["a"];
  ds.slice({Dim::X, 0}).setData("a", item);
  da += nan;
  da.coords()[Dim::X] += nan;
  da.masks()["mask"] += nan;
  da.attrs()[Dim("attr")] += nan;
  ds.slice({Dim::X, 0}).setData("a", item);
}
