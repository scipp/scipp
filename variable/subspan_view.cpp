// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/subspan_view.h"
#include "scipp/core/except.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {

namespace {

/// Helper returning vector of subspans from begin to end
template <class Var, class T>
auto make_subspans(Var &, const ElementArrayView<T> &first,
                   const ElementArrayView<T> &last) {
  const auto len = first.size();
  std::vector<
      span<std::conditional_t<std::is_const_v<Var>, std::add_const_t<T>, T>>>
      spans;
  spans.reserve(len);
  for (scipp::index i = 0; i < len; ++i)
    spans.emplace_back(scipp::span(&first[i], &last[i] + 1));
  return spans;
}

template <class T>
auto make_empty_subspans(const ElementArrayView<T> &, const Dimensions &dims) {
  return std::vector<span<T>>(dims.volume());
}

template <class T>
auto make_subspans(T *base, const Variable &indices,
                   const scipp::index stride) {
  if (stride != 1)
    throw std::logic_error(
        "span only supports stride=1, this should be "
        "unreachable due to an earlier check, may want to generalize this "
        "later to support in particular stride=0 for broadcasted buffers");
  const auto spans = variable::transform<scipp::index_pair>(
      indices, overloaded{core::transform_flags::expect_no_variance_arg<0>,
                          [](const units::Unit &) { return units::one; },
                          [base, stride](const auto &offset) {
                            return scipp::span(base + stride * offset.first,
                                               base + stride * offset.second);
                          }});
  const auto vals = spans.template values<span<T>>().as_span();
  // TODO This is slightly backwards: Extract span from variable, just to put
  // them into another one in make_subspan_view.
  return std::vector<span<T>>(vals.begin(), vals.end());
}

template <class T, class Var, class ValuesMaker, class VariancesMaker>
Variable make_subspan_view(Var &var, const Dimensions &dims,
                           ValuesMaker make_values,
                           VariancesMaker make_variances) {
  std::conditional_t<std::is_const_v<T>, const Var, Var> &var_ref = var;
  using MaybeConstT =
      std::conditional_t<std::is_const_v<Var>, std::add_const_t<T>, T>;
  auto valuesView = make_values(var_ref);
  if (var.hasVariances()) {
    auto variancesView = make_variances(var_ref);
    return makeVariable<span<MaybeConstT>>(
        Dimensions{dims}, var.unit(),
        Values(valuesView.begin(), valuesView.end()),
        Variances(variancesView.begin(), variancesView.end()));
  } else
    return makeVariable<span<MaybeConstT>>(
        Dimensions{dims}, var.unit(),
        Values(valuesView.begin(), valuesView.end()));
}

/// Return Variable containing spans over given dimension as elements.
template <class T, class Var> Variable subspan_view(Var &var, const Dim dim) {
  using E = std::remove_const_t<T>;
  const auto len = var.dims()[dim];
  auto dims = var.dims();
  dims.erase(dim);
  const auto values_view = [dim, len, dims](auto &v) {
    if (len == 0)
      return make_empty_subspans(v.template values<E>(), dims);
    // `Var` may be `const Variable`, ensuring we call correct `values`
    // overload. Without this we may clash with readonly flags.
    Var begin = v.slice({dim, 0});
    Var end = v.slice({dim, len - 1});
    return make_subspans(v, begin.template values<E>(),
                         end.template values<E>());
  };
  const auto variances_view = [dim, len, dims](auto &v) {
    if (len == 0)
      return make_empty_subspans(v.template variances<E>(), dims);
    Var begin = v.slice({dim, 0});
    Var end = v.slice({dim, len - 1});
    return make_subspans(v, begin.template variances<E>(),
                         end.template variances<E>());
  };
  return make_subspan_view<T>(var, dims, values_view, variances_view);
}

/// Return Variable containing spans with extents given by indices over given
/// dimension as elements.
template <class T, class Var>
Variable subspan_view(Var &var, const Dim dim, const Variable &indices) {
  using E = std::remove_const_t<T>;
  const auto values_view = [dim, &indices](auto &v) {
    return make_subspans(v.template values<E>().data(), indices,
                         v.dims().offset(dim));
  };
  const auto variances_view = [dim, &indices](auto &v) {
    return make_subspans(v.template variances<E>().data(), indices,
                         v.dims().offset(dim));
  };
  return make_subspan_view<T>(var, indices.dims(), values_view, variances_view);
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

} // namespace

/// Return Variable containing mutable spans over given dimension as elements.
Variable subspan_view(Variable &var, const Dim dim) {
  return subspan_view_impl(var, dim);
}
/// Return Variable containing const spans over given dimension as elements.
Variable subspan_view(const Variable &var, const Dim dim) {
  return subspan_view_impl(var, dim);
}

Variable subspan_view(Variable &var, const Dim dim, const Variable &indices) {
  return subspan_view_impl(var, dim, indices);
}
Variable subspan_view(const Variable &var, const Dim dim,
                      const Variable &indices) {
  return subspan_view_impl(var, dim, indices);
}

} // namespace scipp::variable
