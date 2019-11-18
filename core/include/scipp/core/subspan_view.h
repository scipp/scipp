// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_SUBSPAN_VIEW_H
#define SCIPP_CORE_SUBSPAN_VIEW_H

#include <scipp/core/variable.h>

namespace scipp::core {

template <class T>
auto make_subspans(const span<T> &data, const scipp::index span_len) {
  const auto len = data.size() / span_len;
  std::vector<span<T>> spans;
  spans.reserve(len);
  for (scipp::index i = 0; i < len; ++i)
    spans.emplace_back(data.subspan(span_len * i, span_len));
  return spans;
}

template <class T, class Var> Variable subspan_view(Var &var, const Dim dim) {
  expect::notSparse(var);
  if (dim != var.dims().inner())
    throw except::DimensionError(
        "View over subspan can only be created for inner dimension.");

  using E = std::remove_const_t<T>;
  constexpr static auto values_view = [](auto &v) {
    return make_subspans(v.template values<E>(), v.dims()[v.dims().inner()]);
  };
  constexpr static auto variances_view = [](auto &v) {
    return make_subspans(v.template variances<E>(), v.dims()[v.dims().inner()]);
  };

  auto dims = var.dims();
  dims.erase(dim);
  std::conditional_t<std::is_const_v<T>, const Var, Var> &var_ref = var;
  using MaybeConstT =
      std::conditional_t<std::is_const_v<Var>, std::add_const_t<T>, T>;
  return makeVariable<span<MaybeConstT>>(
      dims, var.unit(), values_view(var_ref),
      var.hasVariances() ? variances_view(var_ref)
                         : std::vector<span<MaybeConstT>>{});
}

} // namespace scipp::core

#endif // SCIPP_CORE_SUBSPAN_VIEW_H
