// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Igor Gudich

#ifndef SCIPP_VALUES_AND_VARIANCES_STRONG_TYPES_H
#define SCIPP_VALUES_AND_VARIANCES_STRONG_TYPES_H

#include "scipp/core/except.h"
#include "scipp/core/vector.h"
#include <optional>
#include <type_traits>

namespace scipp::core {

template <class T, template <class> class Container, class Tag = void>
struct MoveOnlyOptionalContainer : Tag {
  using type = T;
  std::optional<Container<T>> data;
  MoveOnlyOptionalContainer() = default;
  MoveOnlyOptionalContainer(MoveOnlyOptionalContainer &&) = default;
  MoveOnlyOptionalContainer(const MoveOnlyOptionalContainer &) = delete;
  explicit MoveOnlyOptionalContainer(std::optional<Container<T>> &&container)
      : data(std::move(container)) {}
  template <class U> operator MoveOnlyOptionalContainer<U, Container, Tag>() {
    return data ? MoveOnlyOptionalContainer<U, Container, Tag>(
                      std::optional(Container<U>(data->begin(), data->end())))
                : MoveOnlyOptionalContainer<U, Container, Tag>();
  }
};

struct ValuesTag {};

template <class T>
using Values = MoveOnlyOptionalContainer<T, Vector, ValuesTag>;

struct VariancesTag {};

template <class T>
using Variances = MoveOnlyOptionalContainer<T, Vector, VariancesTag>;

namespace detail {
template<class T1, class T2>
constexpr bool is_values_or_variances_v =
    std::is_base_of_v<ValuesTag, T1> && std::is_base_of_v<ValuesTag, T2> ||
        std::is_base_of_v<VariancesTag, T1> && std::is_base_of_v<VariancesTag, T2>;

template<class T1, class T2> struct is_same_or_values_or_variances {
  static constexpr bool value =
      std::is_same_v<T1, T2> || is_values_or_variances_v<T1, T2>;
};
template<class T1, class T2>
bool constexpr is_same_or_values_or_variances_v =
    is_same_or_values_or_variances<T1, T2>::value;

template<class T, class... Args>
constexpr bool is_type_in_pack_v = std::disjunction<
    is_same_or_values_or_variances<T, std::decay_t<Args>>...>::value;

template<class T, class... Args> struct Indexer {
  template<std::size_t... IS>
  static constexpr auto indexOfCorresponding_impl(std::index_sequence<IS...>) {
    return ((is_same_or_values_or_variances_v<T, Args> * IS) + ...);
  }

  static constexpr auto indexOfCorresponding() {
    return indexOfCorresponding_impl(
        std::make_index_sequence<sizeof...(Args)>{});
  }
};

template<class... Ts> // given types
struct ConstructorArgumentsMatcher {
  template<class... Args> // needed types
  constexpr static void checkArgTypesValid() {
    static_assert((is_type_in_pack_v<Args, Ts...> + ...) == sizeof...(Ts));
  }
  template<class T, class... Args> static T construct(Ts &&... ts) {
    auto tp = std::make_tuple(std::forward<Ts>(ts)...);
    return T(std::forward<Args>(extractArgs<Args, Ts...>(tp))...);
  }

private:
  template<class T, class... Args>
  static decltype(auto) extractArgs(std::tuple<Args...> &tp) {
    if constexpr (!is_type_in_pack_v<T, Ts...>)
      return T{};
    else {
      constexpr auto index = Indexer<T, Args...>::indexOfCorresponding();
      using Type = std::decay_t<decltype(std::get<index>(tp))>;
      if constexpr (std::is_same_v<Type, T>)
        return std::get<index>(tp);
      else {
        if constexpr (is_values_or_variances_v<T, Type>) {
          using T1 = typename T::type;
          using T2 = typename Type::type;
          if constexpr (std::is_convertible_v<T1, T2>) {
            return std::get<index>(tp);
          } else {
            throw except::TypeError("Can't convert " +
                to_string(core::dtype<T1>) + " to " +
                to_string(core::dtype<T2>) + ".");
            return T{}; //fake return usefull type for compiler
          }
        }
      }
    }
  }
};

} //namespace detail
} // namespace scipp::core

#endif // SCIPP_VALUES_AND_VARIANCES_STRONG_TYPES_H
