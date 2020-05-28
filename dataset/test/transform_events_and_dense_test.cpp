// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>
#include <vector>

#include "test_macros.h"

#include "scipp/common/numeric.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/histogram.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

using namespace scipp;
using namespace scipp::core;
using namespace scipp::variable;
using namespace scipp::dataset;

TEST(TransformEventsAndDenseTest, two_args) {
  auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto vals = var.values<event_list<double>>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};

  auto dense = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    Values{1.1, 2.2, 3.3, 4.4});
  auto dense_view = subspan_view(dense, Dim::X);

  const auto result =
      transform<pair_custom_t<std::tuple<event_list<double>, span<double>>>>(
          var, dense_view,
          overloaded{
              [](const auto &a, const auto &b) {
                EXPECT_EQ(b.size(), 2);
                return a;
              },
              [](const units::Unit &a, const units::Unit &) { return a; },
              transform_flags::expect_no_variance_arg<0>,
              transform_flags::expect_no_variance_arg<1>});
  EXPECT_EQ(result, var);
}

TEST(TransformEventsAndDenseTest, three_args) {
  auto var = makeVariable<event_list<double>>(Dims{Dim::Y}, Shape{2});
  auto vals = var.values<event_list<double>>();
  vals[0] = {1, 2, 3};
  vals[1] = {4};

  auto dense = makeVariable<double>(Dims{Dim::Y, Dim::X}, Shape{2, 2},
                                    Values{1.1, 2.2, 3.3, 4.4});
  auto dense_view = subspan_view(dense, Dim::X);

  auto dense_with_variance = makeVariable<double>(
      Dims{Dim::X}, Shape{2}, Values{0.1, 0.2}, Variances{0.3, 0.4});
  auto dense_with_variance_view = subspan_view(dense_with_variance, Dim::X);

  const auto out = transform<
      std::tuple<std::tuple<event_list<double>, span<double>, span<double>>>>(
      var, dense_view, dense_with_variance_view,
      overloaded{
          [](const auto &a, const auto &, const auto &c) {
            if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(c)>>) {
              return std::pair(a, a);
            } else
              return a;
          },
          transform_flags::expect_no_variance_arg<0>,
          transform_flags::expect_no_variance_arg<1>,
          [](const units::Unit &a, const units::Unit &, const units::Unit &) {
            return a;
          }});
  EXPECT_TRUE(out.hasVariances());
  EXPECT_TRUE(equals(out.values<event_list<double>>(),
                     var.values<event_list<double>>()));
  EXPECT_TRUE(equals(out.variances<event_list<double>>(),
                     var.values<event_list<double>>()));
}
