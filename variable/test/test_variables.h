// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <gtest/gtest.h>

#include <numeric>

#include "random.h"

#include "scipp/units/dim.h"
#include "scipp/variable/variable.h"

class DenseVariablesTest : public ::testing::TestWithParam<scipp::Variable> {};
class BinnedVariablesTest : public ::testing::TestWithParam<scipp::Variable> {};

inline auto make_dim_labels(const scipp::index ndim,
                            std::initializer_list<scipp::Dim> choices) {
  assert(ndim <= scipp::size(choices));
  scipp::variable::Dims result(choices);
  result.data.resize(ndim);
  return result;
}

template <class T>
scipp::variable::Variable
make_dense_variable(const scipp::variable::Shape &shape, const bool variances,
                    const T offset = T{0}, const T scale = T{1}) {
  using namespace scipp;
  using namespace scipp::variable;

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
