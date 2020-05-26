// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/subspan_view.h"
#include "scipp/core/except.h"

namespace scipp::variable {

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

  const auto values_view = [dim, len](auto &v) {
    return make_subspans(v.slice({dim, 0}).template values<E>(),
                         v.slice({dim, len - 1}).template values<E>());
  };
  const auto variances_view = [dim, len](auto &v) {
    return make_subspans(v.slice({dim, 0}).template variances<E>(),
                         v.slice({dim, len - 1}).template variances<E>());
  };

  auto dims = var.dims();
  dims.erase(dim);
  std::conditional_t<std::is_const_v<T>, const Var, Var> &var_ref = var;
  using MaybeConstT =
      std::conditional_t<std::is_const_v<Var>, std::add_const_t<T>, T>;
  auto valuesView = values_view(var_ref);
  if (var.hasVariances()) {
    auto variancesView = variances_view(var_ref);
    return makeVariable<span<MaybeConstT>>(
        Dimensions{dims}, var.unit(),
        Values(valuesView.begin(), valuesView.end()),
        Variances(variancesView.begin(), variancesView.end()));
  } else
    return makeVariable<span<MaybeConstT>>(
        Dimensions{dims}, var.unit(),
        Values(valuesView.begin(), valuesView.end()));
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

template <class Var> Variable subspan_view_impl(Var &var, const Dim dim) {
  return invoke_subspan_view<double, float, int64_t, int32_t, bool>(var.dtype(),
                                                                    var, dim);
}

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

} // namespace scipp::variable
