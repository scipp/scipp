// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen

#include "transform_test_helpers.h"

#include "scipp/variable/reduction.h"
#include "scipp/variable/util.h"

using namespace scipp;
using namespace scipp::variable;

namespace scipp::testing {
std::vector<scipp::Shape> shapes(std::optional<scipp::index> ndim) {
  static const std::array all_shapes{
      std::array{Shape{1}, Shape{2}, Shape{3}, Shape{5}, Shape{16}},
      std::array{Shape{1, 1}, Shape{1, 2}, Shape{3, 1}, Shape{2, 8},
                 Shape{5, 7}},
      std::array{Shape{1, 1, 1}, Shape{1, 1, 4}, Shape{1, 5, 1}, Shape{7, 1, 1},
                 Shape{2, 8, 4}}};

  std::vector<Shape> res;
  for (scipp::index n = 1; n < 4; ++n) {
    if (ndim.value_or(n) == n) {
      std::copy(all_shapes.at(n - 1).begin(), all_shapes.at(n - 1).end(),
                std::back_inserter(res));
    }
  }
  return res;
}

std::vector<Variable> irregular_bin_indices_1d() {
  return {makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{0}, Values{}),
          makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{2},
                                   Values{index_pair{0, 1}, index_pair{2, 3}}),
          makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{2},
                                   Values{index_pair{0, 2}, index_pair{2, 3}}),
          makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{2},
                                   Values{index_pair{0, 0}, index_pair{0, 3}}),
          makeVariable<index_pair>(Dims{Dim{"i0"}}, Shape{2},
                                   Values{index_pair{0, 4}, index_pair{4, 4}}),
          makeVariable<index_pair>(
              Dims{Dim{"i0"}}, Shape{3},
              Values{index_pair{0, 1}, index_pair{2, 4}, index_pair{4, 5}}),
          makeVariable<index_pair>(
              Dims{Dim{"i0"}}, Shape{3},
              Values{index_pair{0, 1}, index_pair{1, 2}, index_pair{4, 5}}),
          makeVariable<index_pair>(
              Dims{Dim{"i0"}}, Shape{3},
              Values{index_pair{0, 2}, index_pair{2, 2}, index_pair{2, 3}}),
          makeVariable<index_pair>(
              Dims{Dim{"i0"}}, Shape{3},
              Values{index_pair{0, 1}, index_pair{1, 3}, index_pair{3, 5}})};
}

std::vector<Variable> irregular_bin_indices_2d() {
  return {makeVariable<index_pair>(Dims{Dim{"i0"}, Dim{"i1"}}, Shape{2, 2},
                                   Values{index_pair{0, 2}, index_pair{2, 3},
                                          index_pair{3, 5}, index_pair{5, 6}}),
          makeVariable<index_pair>(Dims{Dim{"i0"}, Dim{"i1"}}, Shape{1, 2},
                                   Values{index_pair{0, 1}, index_pair{1, 4}}),
          makeVariable<index_pair>(Dims{Dim{"i0"}, Dim{"i1"}}, Shape{2, 2},
                                   Values{index_pair{0, 1}, index_pair{2, 4},
                                          index_pair{4, 4}, index_pair{6, 7}}),
          makeVariable<index_pair>(Dims{Dim{"i0"}, Dim{"i1"}}, Shape{0, 0},
                                   Values{})};
}

Dims make_dim_labels(const scipp::index ndim,
                     std::initializer_list<scipp::Dim> choices) {
  assert(ndim <= scipp::size(choices));
  scipp::Dims result(choices);
  result.data.resize(ndim);
  return result;
}

scipp::index volume(const scipp::Shape &shape) {
  return std::accumulate(shape.data.begin(), shape.data.end(), 1,
                         std::multiplies<scipp::index>{});
}

Variable make_regular_bin_indices(const scipp::index size,
                                  const scipp::Shape &shape,
                                  const scipp::index ndim) {
  const auto n_bins = volume(shape);
  std::vector<index_pair> aux(n_bins);
  std::generate(begin(aux), end(aux),
                [lower = scipp::index{0}, d_size = static_cast<double>(size),
                 d_n_bins = static_cast<double>(n_bins)]() mutable {
                  const auto upper = static_cast<scipp::index>(
                      static_cast<double>(lower) + d_size / d_n_bins);
                  const index_pair res{lower, upper};
                  lower = upper;
                  return res;
                });
  aux.back().second = size;
  return makeVariable<index_pair>(
      make_dim_labels(ndim, {Dim{"i0"}, Dim{"i1"}, Dim{"i2"}}), shape,
      Values(aux));
}

scipp::index index_volume(const Variable &indices) {
  if (indices.dims().empty())
    return 0;
  const auto &&[begin, end] = unzip(indices);
  return (max(end) - min(begin)).value<scipp::index>();
}

namespace {
auto make_slices_in(const scipp::index dim,
                    const std::span<const scipp::index> &shape,
                    const std::span<const Dim> dim_labels) {
  std::vector<Slice> out;
  if (scipp::size(shape) > dim && shape[dim] > 1) {
    out.emplace_back(dim_labels[dim], 0, shape[dim] - 1);
    out.emplace_back(dim_labels[dim], 0, shape[dim] / 2);
    out.emplace_back(dim_labels[dim], 2, shape[dim]);
    out.emplace_back(dim_labels[dim], 1);
  }
  return out;
}

void push_slices_in(const scipp::index dim,
                    std::vector<std::vector<Slice>> &out,
                    std::vector<Slice> slices,
                    const std::span<const scipp::index> &shape,
                    const std::span<const Dim> dim_labels) {
  if (dim >= scipp::size(shape))
    return;

  for (auto &&slice : make_slices_in(dim, shape, dim_labels)) {
    slices.push_back(std::forward<decltype(slice)>(slice));
    out.push_back(slices);
    push_slices_in(dim + 1, out, slices, shape, dim_labels);
    slices.pop_back();
  }
  push_slices_in(dim + 1, out, slices, shape, dim_labels);
}
} // namespace

std::vector<std::vector<Slice>>
make_slice_combinations(const std::span<const scipp::index> &shape,
                        const std::initializer_list<Dim> dim_labels) {
  std::vector<std::vector<Slice>> out;
  out.reserve(512); // Should be enough.
  std::vector<Slice> slices;
  slices.reserve(shape.size());
  push_slices_in(0, out, slices, shape, {dim_labels.begin(), dim_labels.end()});
  return out;
}

Variable slice(Variable var, const std::span<const Slice> slices) {
  for (const auto &slice : slices) {
    var = var.slice(slice);
  }
  return var;
}
} // namespace scipp::testing
