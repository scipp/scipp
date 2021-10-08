// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Jan-Lukas Wynen
#include <numeric>

#include "random.h"

#include "scipp/units/dim.h"
#include "scipp/variable/variable.h"

inline std::vector<scipp::Shape>
shapes(std::optional<scipp::index> ndim = std::nullopt) {
  using namespace scipp;
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

inline auto make_dim_labels(const scipp::index ndim,
                            std::initializer_list<scipp::Dim> choices) {
  assert(ndim <= scipp::size(choices));
  scipp::Dims result(choices);
  result.data.resize(ndim);
  return result;
}

template <class T>
scipp::variable::Variable
make_dense_variable(const scipp::Shape &shape, const bool variances,
                    const T offset = T{0}, const T scale = T{1}) {
  using namespace scipp;

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

inline auto make_slices(const scipp::span<const scipp::index> &shape) {
  using namespace scipp;
  using namespace scipp::variable;

  std::vector<Slice> res;
  const std::vector dim_labels{Dim::X, Dim::Y, Dim::Z};
  for (size_t dim = 0; dim < 3; ++dim) {
    if (shape.size() > dim && shape[dim] > 1) {
      res.emplace_back(dim_labels.at(dim), 0, shape[dim] - 1);
      res.emplace_back(dim_labels.at(dim), 0, shape[dim] / 2);
      res.emplace_back(dim_labels.at(dim), 2, shape[dim]);
      res.emplace_back(dim_labels.at(dim), 1);
    }
  }
  return res;
}

inline auto volume(const scipp::Shape &shape) {
  return std::accumulate(shape.data.begin(), shape.data.end(), 1,
                         std::multiplies<scipp::index>{});
}

inline auto make_regular_bin_indices(const scipp::index size,
                                     const scipp::Shape &shape,
                                     const scipp::index ndim) {
  using namespace scipp;

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

template <class T>
scipp::variable::Variable
make_binned_variable(scipp::Shape event_shape, const scipp::Shape &bin_shape,
                     const scipp::index bin_dim, const bool variances) {
  using namespace scipp;

  const auto n_bin = volume(bin_shape);
  // Make events large enough to accommodate all bins.
  event_shape.data.at(bin_dim) *= n_bin;

  const auto buffer = make_dense_variable<T>(event_shape, variances);
  const auto bin_dim_label = buffer.dims().label(bin_dim);
  const auto indices = make_regular_bin_indices(
      event_shape.data.at(bin_dim), bin_shape, scipp::size(bin_shape.data));
  return make_bins(indices, bin_dim_label, copy(buffer));
}