// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
#include <gtest/gtest.h>

#include "scipp/core/dataset.h"

using namespace scipp;
using namespace scipp::core;

static auto make_sparse() {
  auto var = makeVariable<double>({Dim::Y, Dim::X}, {2, Dimensions::Sparse});
  var.setUnit(units::us);
  auto vals = var.sparseValues<double>();
  vals[0] = {1.1, 2.2, 3.3};
  vals[1] = {1.1, 2.2, 3.3, 5.5};
  return var;
}

static auto make_sparse_array_coord_only() {
  return DataArray(std::optional<Variable>(), {{Dim::X, make_sparse()}});
}

static auto make_histogram() {
  auto edges = makeVariable<double>({{Dim::Y, 2}, {Dim::X, 3}}, units::us,
                                    {0, 2, 4, 1, 3, 5});
  auto data = makeVariable<double>({Dim::X, 2}, {2.0, 3.0}, {0.3, 0.4});

  return DataArray(data, {{Dim::X, edges}});
}

TEST(SparseDataOperationsConsistencyTest, multiply) {
  // Apart from uncertainties, the order of operations does not matter. We can
  // either first multiply and then histogram, or first histogram and then
  // multiply.
  const auto sparse = make_sparse_array_coord_only();
  auto edges = makeVariable<double>({Dim::X, 4}, units::us, {1, 2, 3, 4});
  auto data =
      makeVariable<double>({Dim::X, 3}, {2.0, 3.0, 4.0}, {0.3, 0.4, 0.5});

  auto hist = DataArray(data, {{Dim::X, edges}});
  auto ab = histogram(sparse * hist, edges);
  auto ba = histogram(sparse, edges) * hist;

  // Case 1: 1 event per bin => uncertainties are the same
  EXPECT_EQ(ab, ba);

  hist = make_histogram();
  edges = Variable(hist.coords()[Dim::X]);
  ab = histogram(sparse * hist, edges);
  ba = histogram(sparse, edges) * hist;

  // Case 2: Multiple events per bin => uncertainties differ, set to 0 before
  // comparison.
  ab.setVariances(Vector<double>(4));
  ba.setVariances(Vector<double>(4));
  EXPECT_EQ(ab, ba);
}

TEST(SparseDataOperationsConsistencyTest, flatten_and_sum) {
  const auto sparse = make_sparse_array_coord_only();
  auto edges = makeVariable<double>({Dim::X, 3}, units::us, {1, 3, 6});
  EXPECT_EQ(sum(histogram(sparse, edges), Dim::Y),
            histogram(flatten(sparse, Dim::Y), edges));
}
