// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/subspan_view.h"
#include "scipp/core/except.h"

namespace scipp::variable {

namespace {

/// Helper returning vector of subspans from begin to end
template <class T>
auto make_subspans(const ElementArrayView<T> &first,
                   const ElementArrayView<T> &last) {
  const auto len = first.size();
  std::vector<span<T>> spans;
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
auto make_subspans(T *base, const VariableConstView &indices,
                   const scipp::index stride) {
  const auto &offset = indices.values<core::bucket_base::range_type>();
  const auto len = offset.size();
  std::vector<span<T>> spans;
  spans.reserve(len);
  for (scipp::index i = 0; i < len; ++i)
    spans.emplace_back(scipp::span(base + stride * offset[i].first,
                                   base + stride * offset[i].second));
  return spans;
}

template <class T, class Var, class ValuesMaker, class VariancesMaker>
Variable make_subspan_view(Var &var, const Dimensions &dims,
                           ValuesMaker make_values,
                           VariancesMaker make_variances) {
  std::conditional_t<std::is_const_v<T>, const Var, Var> &var_ref = var;
  using MaybeConstT =
      std::conditional_t<std::is_const_v<Var> &&
                             !std::is_same_v<std::decay_t<Var>, VariableView>,
                         std::add_const_t<T>, T>;
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
  // Check that stride in `dim` is 1
  if (len > 0 && var.template values<E>().data() + 1 !=
                     var.slice({dim, 1}).template values<E>().data())
    throw except::DimensionError(
        "View over subspan can only be created for contiguous "
        "range of data.");

  auto dims = var.dims();
  dims.erase(dim);
  const auto values_view = [dim, len, dims](auto &v) {
    return len == 0
               ? make_empty_subspans(v.template values<E>(), dims)
               : make_subspans(v.slice({dim, 0}).template values<E>(),
                               v.slice({dim, len - 1}).template values<E>());
  };
  const auto variances_view = [dim, len, dims](auto &v) {
    return len == 0
               ? make_empty_subspans(v.template variances<E>(), dims)
               : make_subspans(v.slice({dim, 0}).template variances<E>(),
                               v.slice({dim, len - 1}).template variances<E>());
  };
  return make_subspan_view<T>(var, dims, values_view, variances_view);
}

/// Return Variable containing spans with extents given by indices over given
/// dimension as elements.
template <class T, class Var>
Variable subspan_view(Var &var, const Dim dim,
                      const VariableConstView &indices) {
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
Variable subspan_view_impl(Var &var, Args &&... args) {
  return invoke_subspan_view<double, float, int64_t, int32_t, bool,
                             std::string>(var.dtype(), var, args...);
}

} // namespace

/// Return Variable containing mutable spans over given dimension as elements.
Variable subspan_view(Variable &var, const Dim dim) {
  return subspan_view_impl(var, dim);
}
/// Return Variable containing mutable spans over given dimension as elements.
Variable subspan_view(const VariableView &var, const Dim dim) {
  return subspan_view_impl(var, dim);
}
/// Return Variable containing const spans over given dimension as elements.
Variable subspan_view(const VariableConstView &var, const Dim dim) {
  return subspan_view_impl(var, dim);
}

Variable subspan_view(Variable &var, const Dim dim,
                      const VariableConstView &indices) {
  return subspan_view_impl(var, dim, indices);
}
Variable subspan_view(const VariableView &var, const Dim dim,
                      const VariableConstView &indices) {
  return subspan_view_impl(var, dim, indices);
}
Variable subspan_view(const VariableConstView &var, const Dim dim,
                      const VariableConstView &indices) {
  return subspan_view_impl(var, dim, indices);
}

} // namespace scipp::variable
