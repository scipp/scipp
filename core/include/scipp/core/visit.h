// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef VISIT_H
#define VISIT_H

#include <memory>
#include <tuple>
#include <utility>

namespace scipp::core {

template <class T> class VariableConceptT;

namespace visit_detail {
template <class Variant> struct alternatives_are_const_ptr;
template <class T, class... Ts>
struct alternatives_are_const_ptr<std::variant<T, Ts...>> : std::true_type {};
template <class T, class... Ts>
struct alternatives_are_const_ptr<std::variant<std::unique_ptr<T>, Ts...>>
    : std::false_type {};

template <class Variant, class T>
using alternative =
    std::conditional_t<alternatives_are_const_ptr<std::decay_t<Variant>>::value,
                       const VariableConceptT<T> *,
                       std::unique_ptr<VariableConceptT<T>>>;

template <template <class...> class Tuple, class... T, class... V>
static constexpr bool holds_alternatives(Tuple<T...> &&,
                                         const V &... v) noexcept {
  return (std::holds_alternative<alternative<V, T>>(v) && ...);
}

template <template <class...> class Tuple, class... T, class... V>
static constexpr auto get_args(Tuple<T...> &&, V &&... v) noexcept {
  return std::forward_as_tuple(
      std::get<alternative<V, T>>(std::forward<V>(v))...);
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
template <class... T> struct is_tuple<std::pair<T...>> : std::true_type {};
template <class... T> struct is_tuple<std::tuple<T...>> : std::true_type {};

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
template <class... Ts> struct visit_impl {
  template <class F, class... V> static decltype(auto) apply(F &&f, V &&... v) {
    using namespace visit_detail;
    // For a single input or if same type required for all inputs, Ts is not a
    // tuple. In that case we wrap it and expand it to the correct sizeof...(V).
    return invoke<maybe_duplicate<Ts, V...>...>(std::forward<F>(f),
                                                std::forward<V>(v)...);
  }
};
template <class... Ts> auto visit(const std::tuple<Ts...> &) {
  return visit_impl<Ts...>{};
}

} // namespace scipp::core

#endif // VISIT_H
