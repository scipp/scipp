// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/data_array.h"
#include "scipp/dataset/to_unit.h"
#include "scipp/variable/to_unit.h"

using namespace scipp;

auto make_array() {
  return DataArray(makeVariable<double>(Values{2.0}, sc_units::m),
                   {{Dim("coord"), makeVariable<int>(Values{4}, sc_units::s)}},
                   {{"mask1", makeVariable<bool>(Values{true})}});
}

class ToUnitTest
    : public testing::TestWithParam<std::tuple<sc_units::Unit, CopyPolicy>> {
protected:
  DataArray da = make_array();
};

TEST_F(ToUnitTest, conversion_to_same_unit_returns_identical_copy) {
  da.setData(makeVariable<double>(Values{3.0}, sc_units::m));
  EXPECT_EQ(to_unit(da, sc_units::m), da);
}

TEST_F(ToUnitTest, converts_unit_of_data) {
  da.setData(makeVariable<double>(Values{3.0}, sc_units::m));
  const auto result = to_unit(da, sc_units::mm);
  EXPECT_EQ(result.data(), makeVariable<double>(Values{3000.0}, sc_units::mm));
}

TEST_F(ToUnitTest, preserves_masks) {
  da.setData(makeVariable<double>(Values{3.0}, sc_units::m));
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, sc_units::mm);
  EXPECT_EQ(result.masks()["mask"], da.masks()["mask"]);
}

TEST_F(ToUnitTest, with_new_target_unit_copies_buffers_when_default_policy) {
  da.setUnit(sc_units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, sc_units::mm);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest,
       with_new_target_unit_copies_buffers_when_copy_policy_always) {
  da.setUnit(sc_units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, sc_units::mm, CopyPolicy::Always);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest,
       with_new_target_unit_copies_buffers_when_copy_policy_try_avoid) {
  da.setUnit(sc_units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, sc_units::mm, CopyPolicy::TryAvoid);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest, with_same_target_unit_copies_buffers_when_default_policy) {
  da.setUnit(sc_units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, sc_units::m);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest,
       with_same_target_unit_copies_buffers_when_copy_policy_always) {
  da.setUnit(sc_units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, sc_units::m, CopyPolicy::Always);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest,
       with_same_target_unit_shares_buffers_when_copy_policy_try_avoid) {
  da.setUnit(sc_units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, sc_units::m, CopyPolicy::TryAvoid);
  EXPECT_TRUE(result.data().is_same(da.data()));
  EXPECT_TRUE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

INSTANTIATE_TEST_SUITE_P(UnitAndCopyPolicy, ToUnitTest,
                         testing::Combine(testing::Values(sc_units::m,
                                                          sc_units::mm),
                                          testing::Values(CopyPolicy::TryAvoid,
                                                          CopyPolicy::Always)));

TEST_P(ToUnitTest, does_not_affect_coords) {
  da.coords().set(Dim::X, makeVariable<int>(Values{4}, sc_units::s));

  const auto [unit, policy] = GetParam();
  const auto converted = to_unit(da, unit, policy);

  EXPECT_EQ(converted.coords()[Dim::X],
            makeVariable<int>(Values{4}, sc_units::s));
  EXPECT_EQ(converted.coords(), da.coords());
  EXPECT_TRUE(converted.coords()[Dim::X].is_same(da.coords()[Dim::X]));
}
