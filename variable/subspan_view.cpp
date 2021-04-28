// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/subspan_view.h"
#include "scipp/core/except.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

namespace scipp::variable {

namespace {

template <class T>
auto make_subspans(T *base, const Variable &indices,
                   const scipp::index stride) {
  if (stride != 1)
    throw std::logic_error(
        "span only supports stride=1, this should be "
        "unreachable due to an earlier check, may want to generalize this "
        "later to support in particular stride=0 for broadcasted buffers");
  return variable::transform<scipp::index_pair>(
      indices, overloaded{core::transform_flags::expect_no_variance_arg<0>,
                          [](const units::Unit &) { return units::one; },
                          [base, stride](const auto &offset) {
                            return scipp::span(base + stride * offset.first,
                                               base + stride * offset.second);
                          }});
}

/// Return Variable containing spans with extents given by indices over given
/// dimension as elements.
template <class T, class Var>
Variable subspan_view(Var &var, const Dim dim, const Variable &indices) {
  auto subspans = make_subspans(var.template values<T>().data(), indices,
                                var.dims().offset(dim));
  if (var.hasVariances())
    subspans.setVariances(make_subspans(var.template variances<T>().data(),
                                        indices, var.dims().offset(dim)));
  subspans.setUnit(var.unit());
  return subspans;
}

template <class... Ts, class... Args>
auto invoke_subspan_view(const DType dtype, Args &&... args) {
  Variable ret;
  if (!((scipp::dtype<Ts> == dtype
             ? (ret = subspan_view<Ts>(std::forward<Args>(args)...), true)
             : false) ||
        ...))
    throw except::TypeError("Unsupported dtype.");
  return ret;
}

template <class Var, class... Args>
Variable subspan_view_impl(Var &var, const Dim dim, Args &&... args) {
  if (var.strides()[var.dims().index(dim)] != 1)
    throw except::DimensionError(
        "View over subspan can only be created for contiguous "
        "range of data.");
  return invoke_subspan_view<double, float, int64_t, int32_t, bool,
                             core::time_point, std::string, Eigen::Vector3d>(
      var.dtype(), var, dim, args...);
}

Variable make_indices(const Variable &var, const Dim dim) {
  auto dims = var.dims();
  const auto len = dims[dim];
  dims.erase(dim);
  const auto base = len * units::one;
  const auto end = cumsum(broadcast(base, dims));
  return zip(end - base, end);
}

} // namespace

/// Return Variable containing mutable spans over given dimension as elements.
Variable subspan_view(Variable &var, const Dim dim) {
  return subspan_view(var, dim, make_indices(var, dim));
}
/// Return Variable containing const spans over given dimension as elements.
Variable subspan_view(const Variable &var, const Dim dim) {
  return subspan_view(var, dim, make_indices(var, dim));
}

Variable subspan_view(Variable &var, const Dim dim, const Variable &indices) {
  return subspan_view_impl(var, dim, indices);
}
Variable subspan_view(const Variable &var, const Dim dim,
                      const Variable &indices) {
  return subspan_view_impl(var, dim, indices);
}

} // namespace scipp::variable
