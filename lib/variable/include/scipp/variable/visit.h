// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <tuple>
#include <utility>
#include <variant>

#include "scipp/core/bucket.h"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

/// Access wrapper for a variable with known dtype.
///
/// This uses VariableFactory to obtain views of the underlying data type, e.g.,
/// to access the double values for bucket<Variable> or bucket<DataArray>.
/// DataArray is not known in scipp::variable so the dynamic factory is used for
/// decoupling this.
template <class T, class Var> struct VariableAccess {
  VariableAccess(Var &var) : m_var(&var) {}
  using value_type = T;
  Dimensions dims() const { return m_var->dims(); }
  auto values() const { return variableFactory().values<T>(*m_var); }
  auto variances() const { return variableFactory().variances<T>(*m_var); }
  bool has_variances() const { return variableFactory().has_variances(*m_var); }
  Variable clone() const { return copy(*m_var); }
  Var *m_var{nullptr};
};
template <class T, class Var> auto variable_access(Var &var) {
  return VariableAccess<T, Var>(var);
}

namespace visit_detail {

template <template <class...> class Tuple, class... T, class... V>
static bool holds_alternatives(Tuple<T...> &&, const V &... v) noexcept {
  return ((dtype<T> == variableFactory().elem_dtype(v)) && ...);
}

template <template <class...> class Tuple, class... T, class... V>
static auto get_args(Tuple<T...> &&, V &&... v) noexcept {
  return std::tuple(variable_access<T>(v)...);
}

template <class... Tuple, class F, class... V>
decltype(auto) invoke(F &&f, V &&... v) {
  // Determine return type from call based on first set of allowed inputs, this
  // should give either Variable or void.
  using Ret = decltype(
      std::apply(std::forward<F>(f),
                 get_args(std::tuple_element_t<0, std::tuple<Tuple...>>{},
                          std::forward<V>(v)...)));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    if (!((holds_alternatives(Tuple{}, v...)
               ? (ret = std::apply(std::forward<F>(f),
                                   get_args(Tuple{}, std::forward<V>(v)...)),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};
    return ret;
  } else {
    if (!((holds_alternatives(Tuple{}, v...)
               ? (std::apply(std::forward<F>(f),
                             get_args(Tuple{}, std::forward<V>(v)...)),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};
  }
}

template <class> struct is_tuple : std::false_type {};
template <class... T> struct is_tuple<std::tuple<T...>> : std::true_type {};

template <class> struct is_array : std::false_type {};
template <class T, size_t N>
struct is_array<std::array<T, N>> : std::true_type {};

/// Typedef for T if T is a tuple, else std::tuple<T, T, T, ...>, with T
/// replicated sizeof...(V) times.
template <class T, class... V>
using maybe_duplicate =
    std::conditional_t<is_tuple<T>::value, T,
                       std::tuple<std::conditional_t<true, T, V>...>>;
} // namespace visit_detail

/// Apply callable to variants, similar to std::visit.
///
/// Does not generate code for all possible combinations of alternatives,
/// instead the tuples Ts provide a list of type combinations to try.
template <class... Ts> struct visit {
  template <class F, class... V> static decltype(auto) apply(F &&f, V &&... v) {
    using namespace visit_detail;
    // For a single input or if same type required for all inputs, Ts is not a
    // tuple. In that case we wrap it and expand it to the correct sizeof...(V).
    return invoke<maybe_duplicate<Ts, V...>...>(std::forward<F>(f),
                                                std::forward<V>(v)...);
  }
};

} // namespace scipp::variable
