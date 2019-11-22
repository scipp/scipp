// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Igor Gudich

#ifndef SCIPP_VARIABLE_KEYWORD_ARG_CONSTRUCTOR_H
#define SCIPP_VARIABLE_KEYWORD_ARG_CONSTRUCTOR_H

#include "scipp/core/except.h"
#include "scipp/core/vector.h"
#include <type_traits>

namespace scipp::core {

// The structs needed for universal variable constructor are introduced below.
// Tags are used to match the corresponding arguments treating the arbitrary
// order of arguments in the constructor, and not mixing values and variances.
// Structures Values and Variances just forwards the arguments for constructing
// internal variable structure - array storage.

namespace detail {

template <class U> struct vector_like {
  std::vector<U> data;
  template <class... Args>
  vector_like(Args &&... args) : data(std::forward<Args>(args)...) {}
  template <class T>
  vector_like(std::initializer_list<T> init) : data(init.begin(), init.end()) {}
};

struct ValuesTag {};

struct VariancesTag {};

template <class... Ts> auto makeArgsTuple(Ts &&... ts) {
  return std::make_tuple<std::remove_reference_t<Ts>...>(
      std::forward<Ts>(ts)...);
}

template <class T> auto makeArgsTuple(std::initializer_list<T> init) {
  using iter = typename std::initializer_list<T>::iterator;
  return std::make_tuple<iter, iter>(init.begin(), init.end());
}

} // namespace detail

using Shape = detail::vector_like<scipp::index>;
using Dims = detail::vector_like<Dim>;

template <class... Args>
using ArgsTuple = decltype(detail::makeArgsTuple(std::declval<Args>()...));

template <class... Args> struct Values : detail::ValuesTag {
  ArgsTuple<Args...> tuple;
  Values(Args &&... args)
      : tuple(detail::makeArgsTuple(std::forward<Args>(args)...)) {}
  template <class T>
  Values(std::initializer_list<T> init)
      : tuple(detail::makeArgsTuple(init.begin(), init.end())) {}
};
template <class... Args> Values(Args &&... args)->Values<Args...>;
template <class T>
Values(std::initializer_list<T>)
    ->Values<typename std::initializer_list<T>::iterator,
             typename std::initializer_list<T>::iterator>;

template <class... Args>
struct Variances : std::tuple<Args...>, detail::VariancesTag {
  ArgsTuple<Args...> tuple;
  Variances(Args &&... args)
      : tuple(detail::makeArgsTuple(std::forward<Args>(args)...)) {}
  template <class T>
  Variances(std::initializer_list<T> init)
      : tuple(detail::makeArgsTuple(init.begin(), init.end())) {}
};
template <class... Args> Variances(Args &&... args)->Variances<Args...>;
template <class T>
Variances(std::initializer_list<T>)
    ->Variances<typename std::initializer_list<T>::iterator,
                typename std::initializer_list<T>::iterator>;

namespace detail {
template <class Tag, class T> struct has_tag : std::is_base_of<Tag, T> {};

template <class T, class... Args>
constexpr bool is_type_in_pack_v =
    std::disjunction<std::is_same<T, std::decay_t<Args>>...>::value;

template <class Tag, class... Args>
constexpr bool is_tag_in_pack_v =
    std::disjunction<has_tag<Tag, Args>...>::value;

template <class T, class... Args> class Indexer {
  template <std::size_t... IS>
  static constexpr auto indexOfCorresponding_impl(std::index_sequence<IS...>) {
    return ((has_tag<T, Args>::value * IS) + ...);
  }

public:
  static constexpr auto indexOfCorresponding() {
    return indexOfCorresponding_impl(
        std::make_index_sequence<sizeof...(Args)>{});
  }
};

template <class VarT, class... Ts> class ConstructorArgumentsMatcher {
public:
  template <class... NonDataTypes> constexpr static void checkArgTypesValid() {
    constexpr int nonDataTypesCount =
        (is_type_in_pack_v<NonDataTypes, Ts...> + ...);
    constexpr bool hasVal = is_tag_in_pack_v<ValuesTag, Ts...>;
    constexpr bool hasVar = is_tag_in_pack_v<VariancesTag, Ts...>;
    static_assert(
        nonDataTypesCount + hasVal + hasVar == sizeof...(Ts),
        "Arguments: units::Unit, Shape, Dims, Values and Variances could only "
        "be used. Example: Variable(dtype<float>, units::Unit(units::kg), "
        "Shape{1, 2}, Dims{Dim::X, Dim::Y}, Values({3, 4}))");
  }

  template <class... NonDataTypes> static auto extractArguments(Ts &&... ts) {
    auto tp = std::make_tuple(std::forward<Ts>(ts)...);
    return std::make_tuple(
        extractTagged<ValuesTag, Ts...>(tp),
        extractTagged<VariancesTag, Ts...>(tp),
        std::tuple<NonDataTypes...>(extractArgs<NonDataTypes, Ts...>(tp)...));
  }

  template <class ElemT, class... ValArgs, class... VarArgs,
            class... NonDataTypes>
  static VarT construct(std::tuple<ValArgs...> &&valArgs,
                        std::tuple<VarArgs...> &&varArgs,
                        std::tuple<NonDataTypes...> &&nonData) {

    constexpr bool hasVal = sizeof...(ValArgs);
    constexpr bool hasVar = sizeof...(VarArgs);
    constexpr bool constrVal =
        std::is_constructible_v<Vector<ElemT>, ValArgs...>;
    constexpr bool constrVar =
        std::is_constructible_v<Vector<ElemT>, VarArgs...>;

    if constexpr ((hasVal && !constrVal) || (hasVar && !constrVar))
      throw except::TypeError("Can't create the Variable with type " +
                              to_string(core::dtype<ElemT>) +
                              " with such values and/or variances.");
    else {
      std::optional<Vector<ElemT>> values;
      if (hasVal)
        values = std::make_from_tuple<Vector<ElemT>>(std::move(valArgs));
      std::optional<Vector<ElemT>> variances;
      if (hasVar)
        variances = std::make_from_tuple<Vector<ElemT>>(std::move(varArgs));
      return VarT::template create<ElemT>(
          std::move(std::get<NonDataTypes>(nonData))..., std::move(values),
          std::move(variances));
    }
  }

private:
  template <class T, class... Args>
  static auto extractArgs(std::tuple<Args...> &tp) {
    if constexpr (!is_type_in_pack_v<T, Ts...>)
      return T{};
    else
      return std::move(std::get<T>(tp));
  }

  template <class Tag, class... Args>
  static auto extractTagged(std::tuple<Args...> &tp) {
    if constexpr (!is_tag_in_pack_v<Tag, Ts...>)
      return std::tuple{};
    else {
      constexpr auto index = Indexer<Tag, Args...>::indexOfCorresponding();
      return std::move(std::get<index>(tp).tuple);
    }
  }
};

} // namespace detail
} // namespace scipp::core

#endif // SCIPP_VARIABLE_KEYWORD_ARG_CONSTRUCTOR_H
