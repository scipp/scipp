// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include <numeric>

#include "random.h"

#include "scipp/units/dim.h"
#include "scipp/variable/variable.h"

namespace scipp::testing {
std::vector<scipp::Shape>
shapes(std::optional<scipp::index> ndim = std::nullopt);

std::vector<Variable> irregular_bin_indices_1d();

std::vector<Variable> irregular_bin_indices_2d();

scipp::index volume(const scipp::Shape &shape);

Dims make_dim_labels(scipp::index ndim,
                     std::initializer_list<scipp::Dim> choices);

Variable make_regular_bin_indices(scipp::index size, const scipp::Shape &shape,
                                  scipp::index ndim);

scipp::index index_volume(const Variable &indices);

std::vector<std::vector<Slice>>
make_slice_combinations(const std::span<const scipp::index> &shape,
                        const std::initializer_list<Dim> dim_labels);

Variable slice(Variable var, std::span<const Slice> slices);

template <class T>
scipp::variable::Variable
make_dense_variable(const scipp::Shape &shape, const bool variances,
                    const T offset = T{0}, const T scale = T{1}) {
  const auto ndim = scipp::size(shape.data);
  const auto dims = make_dim_labels(ndim, {Dim::X, Dim::Y, Dim::Z});
  auto var = variances ? makeVariable<T>(dims, shape, Values{}, Variances{})
                       : makeVariable<T>(dims, shape, Values{});

  const auto total_size = var.dims().volume();
  std::generate(
      var.template values<T>().begin(), var.template values<T>().end(),
      [x = -scale * (total_size / 2.0 + offset), total_size, offset,
       scale]() mutable { return x += scale * (1.0 / total_size + offset); });
  if (variances) {
    std::generate(var.template variances<T>().begin(),
                  var.template variances<T>().end(),
                  [x = -scale * (total_size / 20.0 + offset), total_size,
                   offset, scale]() mutable {
                    return x += scale * (10.0 / total_size + offset);
                  });
  }
  return var;
}

template <class T>
scipp::variable::Variable
make_binned_variable(scipp::Shape event_shape, const scipp::Shape &bin_shape,
                     const scipp::index bin_dim, const bool variances,
                     const T offset = T{0}, const T scale = T{1}) {
  const auto n_bin = volume(bin_shape);
  // Make events large enough to accommodate all bins.
  event_shape.data.at(bin_dim) *= n_bin;

  const auto buffer =
      make_dense_variable<T>(event_shape, variances, offset, scale);
  const auto bin_dim_label = buffer.dims().label(bin_dim);
  const auto indices = make_regular_bin_indices(
      event_shape.data.at(bin_dim), bin_shape, scipp::size(bin_shape.data));
  return make_bins(indices, bin_dim_label, copy(buffer));
}
} // namespace scipp::testing
