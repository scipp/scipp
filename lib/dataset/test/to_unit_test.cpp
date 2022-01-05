// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/data_array.h"
#include "scipp/dataset/to_unit.h"
#include "scipp/variable/to_unit.h"

using namespace scipp;

class ToUnitDataArrayTest : public ::testing::Test {
protected:
  const DataArray data_array =
      DataArray(makeVariable<double>(Dims{Dim::X}, Shape{3},
                                     Values{1.0, 2.0, 3.0}, units::m),
                {{Dim::X, makeVariable<int>(Dims{Dim::X}, Shape{3},
                                            Values{4, 5, 6}, units::s)}},
                {{"m", makeVariable<bool>(Dims{Dim::X}, Shape{3},
                                          Values{true, false, true})}});

  static void do_check(const DataArray &original, const units::Unit target_unit,
                       const CopyPolicy copy_policy, const bool expect_copy) {
    const auto converted = to_unit(original, target_unit, copy_policy);

    EXPECT_EQ(converted.data(), to_unit(original.data(), target_unit));
    EXPECT_EQ(converted.coords()[Dim::X].unit(), units::s);
    EXPECT_EQ(converted.masks(), original.masks());

    EXPECT_TRUE(converted.coords()[Dim::X].is_same(original.coords()[Dim::X]));
    EXPECT_EQ(converted.data().is_same(original.data()), !expect_copy);
    EXPECT_EQ(converted.masks()["m"].is_same(original.masks()["m"]),
              !expect_copy);
  }
};

TEST_F(ToUnitDataArrayTest, different_unit) {
  do_check(data_array, units::mm, CopyPolicy::TryAvoid, true);
  do_check(data_array, units::mm, CopyPolicy::Always, true);
}

TEST_F(ToUnitDataArrayTest, same_unit) {
  do_check(data_array, units::m, CopyPolicy::TryAvoid, false);
  do_check(data_array, units::m, CopyPolicy::Always, true);
}

auto make_array() {
  return DataArray(makeVariable<double>(Values{2.0}, units::m),
                   {{Dim("coord"), makeVariable<int>(Values{4}, units::s)}},
                   {{"mask1", makeVariable<bool>(Values{true})}},
                   {{Dim("attr"), makeVariable<int>(Values{7}, units::kg)}});
}

class ToUnitTest
    : public testing::TestWithParam<std::tuple<units::Unit, CopyPolicy>> {
protected:
  DataArray da = make_array();
};

TEST_F(ToUnitTest, converts_unit_of_data) {
  // TODO Use DataArrayBuilder
  da.setData(makeVariable<double>(Values{3.0}, units::m));
  const auto result = to_unit(da, units::mm);
  EXPECT_EQ(result.data(), makeVariable<double>(Values{3000.0}, units::mm));
  // const auto result = to_unit(da, units::m);
  // EXPECT_EQ(result.data(), makeVariable<double>(Values{3.0}, units::m));
}

/* corresponds to three tests below
TEST_F(ToUnitTest, with_same_target_unit_buffer_sharing_depends_on_policy) {
  auto da = make_array();
  da.setData(makeVariable<double>(Values{3.0}, units::m));
  EXPECT_FALSE(to_unit(da, units::m).data().is_same(da.data()));
  EXPECT_FALSE(
      to_unit(da, units::m, CopyPolicy::Always).data().is_same(da.data()));
  EXPECT_TRUE(
      to_unit(da, units::m, CopyPolicy::TryAvoid).data().is_same(da.data()));
}
// cons:
// - fresh state
// - clear naming
// pros:
// - harder to compare the 3 cases at a glance

*/

// TODO mask values

// If we don't split this, a brief glance may lead us to think that a multi-step
// process is being tested
//
// What about setup duplication?
// - cheap to developer (copy and paste easier than writing test fixture)
// - if tests are good, they should not need to be changed (or read) often or at
// all
// => not a problem?

// Did we go too far? Is having similar asserts in adjacent lines valuable?

// Adding new behavior:
// - new test is beneficial, e.g., simpler code review
// - same for changing behavior
//
// - Changing behavior: Would imply changing test name, serves as a safety check
//   that code change was intentional?
//
// Finding bugs:
// - Single failing tests easier to parse

TEST_F(ToUnitTest, with_new_target_unit_copies_buffers_when_default_policy) {
  da.setUnit(units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, units::mm);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest,
       with_new_target_unit_copies_buffers_when_copy_policy_always) {
  da.setUnit(units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, units::mm, CopyPolicy::Always);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest,
       with_new_target_unit_copies_buffers_when_copy_policy_try_avoid) {
  da.setUnit(units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, units::mm, CopyPolicy::TryAvoid);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest, with_same_target_unit_copies_buffers_when_default_policy) {
  da.setUnit(units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, units::m);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest,
       with_same_target_unit_copies_buffers_when_copy_policy_always) {
  da.setUnit(units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, units::m, CopyPolicy::Always);
  EXPECT_FALSE(result.data().is_same(da.data()));
  EXPECT_FALSE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

TEST_F(ToUnitTest,
       with_same_target_unit_shares_buffers_when_copy_policy_try_avoid) {
  da.setUnit(units::m);
  da.masks().set("mask", makeVariable<bool>(Values{true}));
  const auto result = to_unit(da, units::m, CopyPolicy::TryAvoid);
  EXPECT_TRUE(result.data().is_same(da.data()));
  EXPECT_TRUE(result.masks()["mask"].is_same(da.masks()["mask"]));
}

INSTANTIATE_TEST_SUITE_P(UnitAndCopyPolicy, ToUnitTest,
                         testing::Combine(testing::Values(units::m, units::mm),
                                          testing::Values(CopyPolicy::TryAvoid,
                                                          CopyPolicy::Always)));

TEST_P(ToUnitTest, does_not_affect_coords) {
  da.coords().set(Dim::X, makeVariable<int>(Values{4}, units::s));

  const auto [unit, policy] = GetParam();
  const auto converted = to_unit(da, unit, policy);

  EXPECT_EQ(converted.coords()[Dim::X], makeVariable<int>(Values{4}, units::s));
  EXPECT_EQ(converted.coords(), da.coords());
  EXPECT_TRUE(converted.coords()[Dim::X].is_same(da.coords()[Dim::X]));
}

TEST_P(ToUnitTest, does_not_affect_attrs) {
  da.attrs().set(Dim::X, makeVariable<int>(Values{4}, units::s));

  const auto [unit, policy] = GetParam();
  const auto converted = to_unit(da, unit, policy);

  EXPECT_EQ(converted.attrs()[Dim::X], makeVariable<int>(Values{4}, units::s));
  EXPECT_EQ(converted.attrs(), da.attrs());
  EXPECT_TRUE(converted.attrs()[Dim::X].is_same(da.attrs()[Dim::X]));
}

// TEST(ToUnitTest, is_applied_to_data) {
//  const auto da = DataArray(makeVariable<double>(Values{2.0}, units::m));
//  const auto result = to_unit(da, units::mm, CopyPolicy::TryAvoid);
//  EXPECT_EQ(converted.data().is_same(original.data()));
//}
