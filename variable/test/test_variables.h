// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <gtest/gtest.h>

#include <numeric>

#include "scipp/units/dim.h"
#include "scipp/variable/variable.h"

class DenseVariablesTest : public ::testing::TestWithParam<scipp::Variable> {};
class BinnedVariablesTest : public ::testing::TestWithParam<scipp::Variable> {};

inline auto make_dim_labels(const scipp::index ndim,
                            std::initializer_list<scipp::units::Dim> choices) {
  assert(ndim <= scipp::size(choices));
  scipp::variable::Dims result(choices);
  result.data.resize(ndim);
  return result;
}

template <class T>
scipp::variable::Variable
make_dense_variable(const scipp::variable::Shape &shape, const bool variances) {
  using namespace scipp;
  using namespace scipp::variable;

  const auto ndim = scipp::size(shape.data);
  const auto dims = make_dim_labels(ndim, {Dim::X, Dim::Y, Dim::Z});
  auto var = variances ? makeVariable<T>(dims, shape, Values{}, Variances{})
                       : makeVariable<T>(dims, shape, Values{});

  const auto total_size = var.dims().volume();
  std::iota(var.template values<T>().begin(), var.template values<T>().end(),
            -total_size / 2.0);
  if (variances) {
    std::generate(var.template variances<T>().begin(),
                  var.template variances<T>().end(),
                  [x = -total_size / 20.0, total_size]() mutable {
                    return x += 10.0 / total_size;
                  });
  }
  return var;
}
