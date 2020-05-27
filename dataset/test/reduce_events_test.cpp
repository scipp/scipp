// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/dataset/reduction.h"
#include "scipp/variable/reduction.h"

using namespace scipp;
using namespace scipp::dataset;

namespace {
auto make_events() {
  auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{3});
  const auto &var_ = var.values<event_list<double>>();
  var_[0] = {1, 2, 3};
  var_[1] = {4, 5};
  var_[2] = {6, 7};
  return var;
}
} // namespace

TEST(ReduceEventsTest, flatten_fail) {
  EXPECT_THROW(static_cast<void>(flatten(make_events(), Dim::X)),
               except::DimensionError);
  EXPECT_THROW(static_cast<void>(flatten(make_events(), Dim::Z)),
               except::DimensionError);
}

TEST(ReduceEventsTest, flatten) {
  auto expected = makeVariable<event_list<double>>(
      Dims{}, Shape{}, Values{event_list<double>{1, 2, 3, 4, 5, 6, 7}});
  EXPECT_EQ(flatten(make_events(), Dim::Y), expected);
}

TEST(ReduceEventsTest, flatten_dataset_with_mask) {
  Dataset d;
  d.setMask("y", makeVariable<bool>(Dims{Dim::Y}, Shape{3},
                                    Values{false, true, false}));
  d.coords().set(Dim::X, make_events());
  d.coords().set(Dim("label"), make_events());
  d.setData("b", make_events());
  auto expected = makeVariable<event_list<double>>(
      Dims{}, Shape{}, Values{event_list<double>{1, 2, 3, 6, 7}});

  const auto flat = flatten(d, Dim::Y);

  EXPECT_EQ(flat["b"].coords()[Dim::X], expected);
  EXPECT_EQ(flat["b"].coords()[Dim("label")], expected);
  EXPECT_EQ(flat["b"].data(), expected);
}

TEST(ReduceEventsTest, flatten_dataset_non_constant_scalar_weight_fail) {
  Dataset d;
  d.coords().set(Dim::X, make_events());
  d.setData("b", makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 2, 3}));
  EXPECT_THROW(flatten(d, Dim::Y), except::EventDataError);
  d.setData("b", makeVariable<double>(Dims{Dim::Y}, Shape{3}, Values{1, 1, 1}));
  EXPECT_NO_THROW(flatten(d, Dim::Y));
}
