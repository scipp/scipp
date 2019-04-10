/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
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
  using Ret = decltype(
      std::invoke(std::forward<F>(f), std::get<0>(std::forward<Variant>(v))));

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
// template <class F, class Variant> decltype(auto) visit(F &&f, Variant &&var)
// {
//  return invoke_active(
//      std::forward<F>(f), std::forward<Variant>(var),
//      std::make_index_sequence<
//          std::variant_size_v<std::remove_reference_t<Variant>>>{});
//}

template <class F, class V1, class V2, class... T1, class... T2>
decltype(auto) invoke_active(F &&f, V1 &&v1, V2 &&v2, const std::tuple<T1...> &,
                             const std::tuple<T2...> &) {
  using Ret = decltype(std::invoke(std::forward<F>(f),
                                   std::get<0>(std::forward<V1>(v1)),
                                   std::get<0>(std::forward<V2>(v2))));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    // For now we only support same type in both variants. Eventually this will
    // be generalized.
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
// template <class F, class Variant> decltype(auto) visit(F &&f, Variant &&var)
// {
//  return invoke_active(
//      std::forward<F>(f), std::forward<Variant>(var),
//      std::make_index_sequence<
//          std::variant_size_v<std::remove_reference_t<Variant>>>{});
//}

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
};
template <class... Ts> auto visit(const std::tuple<Ts...> &) {
  return visit_impl<Ts...>{};
}

// template <class F, class Variant, class Indices>
// decltype(auto) visit(F &&f, Variant &&var, Indices indices) {
//  return invoke_active(std::forward<F>(f), std::forward<Variant>(var),
//  indices);
//}

/*
template <class T1, class T2> struct multiplies {
  constexpr auto operator()(const T1 &lhs, const T2 &rhs) const {
    return lhs * rhs;
  }
};

template <class T1, class T2> struct divides {
  constexpr auto operator()(const T1 &lhs, const T2 &rhs) const {
    return lhs / rhs;
  }
};

// All alternative pairs (T1, T2) where Op(T1, T2) gives a valid alternative.
template <template <class, class> class Op, class T1, class T2>
constexpr auto product_valid_impl() {
  constexpr T1 x;
  constexpr T2 y;
  if constexpr (isKnownUnit(Op<T1, T2>()(x, y)))
    return std::tuple<std::pair<T1, T2>>{};
  else
    return std::tuple<>{};
}
template <template <class, class> class Op, class T1, class... T2>
constexpr auto product_valid(std::tuple<T2...>) {
  return std::tuple_cat(product_valid_impl<Op, T1, T2>()...);
}

template <template <class, class> class Op, class... T1, class... T2>
constexpr auto expand(const std::variant<T1...> &,
                      const std::variant<T2...> &) {
  return std::tuple_cat(product_valid<Op, T1>(std::tuple<T2...>{})...);
}

template <class F, class Variant, class... T1, class... T2>
decltype(auto) invoke_active(F &&f, Variant &&v1, Variant &&v2,
                             std::tuple<std::pair<T1, T2>...>) {
  using Ret = decltype(std::invoke(std::forward<F>(f),
                                   std::get<0>(std::forward<Variant>(v1)),
                                   std::get<0>(std::forward<Variant>(v2))));

  if constexpr (!std::is_same_v<void, Ret>) {
    Ret ret;
    if (!((std::holds_alternative<T1>(v1) && std::holds_alternative<T2>(v2)
               ? (ret = std::invoke(std::forward<F>(f),
                                    std::get<T1>(std::forward<Variant>(v1)),
                                    std::get<T2>(std::forward<Variant>(v2))),
                  true)
               : false) ||
          ...))
      throw std::bad_variant_access{};

    return ret;
  } else {
    throw std::runtime_error("???");
  }
}

template <template <class, class> class Op, class F, class Variant>
decltype(auto) visit(F &&f, Variant &&var1, Variant &&var2) {
  constexpr auto indices = expand<Op>(var1, var2);
  return invoke_active(std::forward<F>(f), std::forward<Variant>(var1),
                       std::forward<Variant>(var2), indices);
}

template <class T> struct always_false : std::false_type {};

// Mutliplying two units together using std::visit to run through the contents
// of the std::variant
// Unit operator*(const Unit &a, const Unit &b) {
//  try {
// return Unit(myvisit<multiplies>(
*/

} // namespace scipp::core
