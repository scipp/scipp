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

template <class... Ts> struct pair_self {
  using type = std::tuple<std::pair<Ts, Ts>...>;
};
// template <class... Ts> struct pair_product {
//  using type = decltype(std::tuple_cat(std::tuple<std::pair<Ts,
//  Ts...>>{}...));
//};
template <class... Ts> struct pair_custom { using type = std::tuple<Ts...>; };

template <class... Ts> using pair_self_t = typename pair_self<Ts...>::type;
// template <class... Ts>
// using pair_product_t = typename pair_product<Ts...>::type;
template <class... Ts> using pair_custom_t = typename pair_custom<Ts...>::type;

template <class F, class Variant, class... Ts>
decltype(auto) invoke_active(F &&f, Variant &&v, const std::tuple<Ts...> &) {
  using Ret = decltype(std::invoke(
      std::forward<F>(f), std::get<std::tuple_element_t<0, std::tuple<Ts...>>>(
                              std::forward<Variant>(v))));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    if (!((std::holds_alternative<Ts>(v)
               ? (ret = std::invoke(std::forward<F>(f),
                                    std::get<Ts>(std::forward<Variant>(v))),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};

    return ret;
  } else {
    if (!((std::holds_alternative<Ts>(v)
               ? (std::invoke(std::forward<F>(f),
                              std::get<Ts>(std::forward<Variant>(v))),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};
  }
}

template <class F, class V1, class V2, class... T1, class... T2>
decltype(auto) invoke_active(F &&f, V1 &&v1, V2 &&v2, const std::tuple<T1...> &,
                             const std::tuple<T2...> &) {
  using Ret = decltype(std::invoke(std::forward<F>(f),
                                   std::get<0>(std::forward<V1>(v1)),
                                   std::get<0>(std::forward<V2>(v2))));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    if (!((std::holds_alternative<T1>(v1) && std::holds_alternative<T2>(v2)
               ? (ret = std::invoke(std::forward<F>(f),
                                    std::get<T1>(std::forward<V1>(v1)),
                                    std::get<T2>(std::forward<V2>(v2))),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};

    return ret;
  } else {
    if (!((std::holds_alternative<T1>(v1) && std::holds_alternative<T2>(v2)
               ? (std::invoke(std::forward<F>(f),
                              std::get<T1>(std::forward<V1>(v1)),
                              std::get<T2>(std::forward<V2>(v2))),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};
  }
}

template <class T, class... V> bool holds_alternative(V &&... v) {
  return (std::holds_alternative<T>(v) && ...);
}
template <class T, class F, class... V> auto invoke(F &&f, V &&... v) {
  return std::invoke(std::forward<F>(f), std::get<T>(std::forward<V>(v))...);
}

template <class F, class... Ts, class... V>
decltype(auto) invoke_active(F &&f, const std::tuple<Ts...> &, V &&... v) {
  using Ret = decltype(std::invoke(
      std::forward<F>(f), std::get<std::tuple_element_t<0, std::tuple<Ts...>>>(
                              std::forward<V>(v))...));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    // For now we only support same type in both variants. Eventually this will
    // be generalized.
    if (!((holds_alternative<Ts>(v...)
               ? (ret = invoke<Ts>(std::forward<F>(f), std::forward<V>(v)...),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};

    return ret;
  } else {
    if (!((holds_alternative<Ts>(v...)
               ? (invoke<Ts>(std::forward<F>(f), std::forward<V>(v)...), true)
               : false) ||
          ...))
      throw std::bad_variant_access{};
  }
}

template <class T> class VariableConceptT;
template <class T> using alternative = std::unique_ptr<VariableConceptT<T>>;
template <class... Ts> struct visit_impl {
  template <class F, class Variant>
  static decltype(auto) apply(F &&f, Variant &&var) {
    return invoke_active(std::forward<F>(f), std::forward<Variant>(var),
                         std::tuple<alternative<Ts>...>());
  }
  template <class F, class V1, class V2>
  static decltype(auto) apply(F &&f, V1 &&v1, V2 &&v2) {
    return invoke_active(
        std::forward<F>(f), std::forward<V1>(v1), std::forward<V2>(v2),
        std::tuple<alternative<typename Ts::first_type>...>(),
        std::tuple<alternative<typename Ts::second_type>...>());
  }
  // Arbitrary number of variants, but only same alternative for all supported.
  template <class F, class... V> static decltype(auto) apply(F &&f, V &&... v) {
    return invoke_active(std::forward<F>(f), std::tuple<alternative<Ts>...>(),
                         std::forward<V>(v)...);
  }
};
template <class... Ts> auto visit(const std::tuple<Ts...> &) {
  return visit_impl<Ts...>{};
}

} // namespace scipp::core

#endif // VISIT_H
