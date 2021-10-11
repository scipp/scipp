// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "transform_test_helpers.h"

using namespace scipp;
using namespace scipp::variable;

namespace scipp::testing {
namespace {
auto make_slices_in(const scipp::index dim,
                    const scipp::span<const scipp::index> &shape) {
  static const std::vector dim_labels{Dim::X, Dim::Y, Dim::Z};
  std::vector<Slice> out;
  if (scipp::size(shape) > dim && shape[dim] > 1) {
    out.emplace_back(dim_labels.at(dim), 0, shape[dim] - 1);
    out.emplace_back(dim_labels.at(dim), 0, shape[dim] / 2);
    out.emplace_back(dim_labels.at(dim), 2, shape[dim]);
    out.emplace_back(dim_labels.at(dim), 1);
  }
  return out;
}

void push_slices_in(const scipp::index dim,
                    std::vector<std::vector<Slice>> &out,
                    std::vector<Slice> slices,
                    const scipp::span<const scipp::index> &shape) {
  if (dim >= scipp::size(shape))
    return;

  for (auto &&slice : make_slices_in(dim, shape)) {
    slices.push_back(std::forward<decltype(slice)>(slice));
    out.push_back(slices);
    push_slices_in(dim + 1, out, slices, shape);
    slices.pop_back();
  }
  push_slices_in(dim + 1, out, slices, shape);
}
} // namespace

std::vector<std::vector<Slice>>
make_slice_combinations(const scipp::span<const scipp::index> &shape) {
  std::vector<std::vector<Slice>> out;
  out.reserve(128); // Should be enough.
  std::vector<Slice> slices;
  slices.reserve(shape.size());
  push_slices_in(0, out, slices, shape);
  return out;
}
} // namespace scipp::testing