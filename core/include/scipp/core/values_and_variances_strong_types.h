// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Igor Gudich

#ifndef SCIPP_VALUES_AND_VARIANCES_STRONG_TYPES_H
#define SCIPP_VALUES_AND_VARIANCES_STRONG_TYPES_H

#include "scipp/core/vector.h"
#include <optional>
#include <type_traits>

namespace scipp::core {
struct ValuesMark {};

template <class T> struct Values : ValuesMark {
  using type = T;
  std::optional<Vector<T>> values;
  Values() = default;
  Values(Values &&) = default;
  Values(const Values &) = delete;
  explicit Values(std::optional<Vector<T>> &&v) : values(std::move(v)) {}
  template <class... Ts>
  explicit Values(Ts &&... args) : values(std::forward<Ts>(args)...) {}

  template <class OtherT> operator Values<OtherT>() {
    return Values<OtherT>{
        std::optional{Vector<OtherT>(values->begin(), values->end())}};
  }
};
template <class T> Values(std::optional<Vector<T>> &&)->Values<T>;

struct VariancesMark {};

template <class T> struct Variances : VariancesMark {
  using type = T;
  std::optional<Vector<T>> variances;
  Variances() = default;
  Variances(Variances &&) = default;
  Variances(const Variances &) = delete;
  explicit Variances(std::optional<Vector<T>> &&v) : variances(std::move(v)) {}
  template <class... Ts>
  explicit Variances(Ts &&... args) : variances(std::forward<Ts>(args)...) {}

  template <class OtherT> operator Variances<OtherT>() {
    return Variances<OtherT>{
        std::optional{Vector<OtherT>(variances->begin(), variances->end())}};
  }
};
template <class T> Variances(std::optional<Vector<T>> &&)->Variances<T>;

template <class T1, class T2>
constexpr bool is_values_or_variances_v =
    std::is_base_of_v<ValuesMark, T1> &&std::is_base_of_v<ValuesMark, T2> ||
    std::is_base_of_v<VariancesMark, T1> &&std::is_base_of_v<VariancesMark, T2>;

template <class T1, class T2> struct is_same_or_values_or_variances {
  static constexpr bool value =
      std::is_same_v<T1, T2> || is_values_or_variances_v<T1, T2>;
};

template <class T, class... Args>
using hasType =
    std::disjunction<is_same_or_values_or_variances<T, std::decay_t<Args>>...>;

template <class T, class... Args> struct Indexer {
  template <std::size_t... IS>
  static constexpr auto indexOfCorresponding_impl(std::index_sequence<IS...>) {
    return ((is_same_or_values_or_variances<T, Args>::value * IS) + ...);
  }

  static constexpr auto indexOfCorresponding() {
    return indexOfCorresponding_impl(
        std::make_index_sequence<sizeof...(Args)>{});
  }
};

template <class... Ts> // given types
struct ConstructorArgumentsMatcher {
  template <class... Args> // needed types
  constexpr static void checkArgTypesValid() {
    static_assert((hasType<Args, Ts...>::value + ...) == sizeof...(Ts));
  }
  template <class T, class... Args> static T construct(Ts &&... ts) {
    auto tp = std::make_tuple(std::forward<Ts>(ts)...);
    return T(std::forward<Args>(extractArgs<Args, Ts...>(tp))...);
  }

private:
  template <class T, class... Args>
  static decltype(auto) extractArgs(std::tuple<Args...> &tp) {
    if constexpr (!hasType<T, Ts...>::value)
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
            return T{};
          }
        }
      }
    }
  }
};

} // namespace scipp::core

#endif // SCIPP_VALUES_AND_VARIANCES_STRONG_TYPES_H
