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

// The structs needed for universal variable constructor are introduced below.
// Tags are used to match the corresponding arguments treating the arbitrary
// order of arguments in the constructor, and not mixing values and variances.
// Functions Values and Variances just forwards the arguments for constructing
// internal variable structure - array storage.

struct Shape {
  std::vector<scipp::index> shape;
  template <class... Args>
  Shape(Args &&... args) : shape(std::forward<Args>(args)...) {}
  template <class T>
  Shape(std::initializer_list<T> init) : shape(init.begin(), init.end()) {}
};

struct Dims {
  std::vector<Dim> dims;
  template <class... Args>
  Dims(Args &&... args) : dims(std::forward<Args>(args)...) {}
  template <class T>
  Dims(std::initializer_list<T> init) : dims(init.begin(), init.end()) {}
};

template <class Tag, class... Ts> struct TaggedTuple {
  using tag_type = Tag;
  using tuple_type = std::tuple<Ts...>;
  tag_type tag;
  tuple_type tuple;
};

struct ValuesTag {};

template <class... Ts> auto values(Ts &&... ts) noexcept {
  auto res = TaggedTuple<ValuesTag, std::remove_reference_t<Ts>...>{
      {}, std::make_tuple(std::forward<Ts>(ts)...)};
  return res;
}

template <class T> auto values(std::initializer_list<T> init) noexcept {
  using iter = typename std::initializer_list<T>::iterator;
  return TaggedTuple<ValuesTag, iter, iter>{
      {}, std::make_tuple(init.begin(), init.end())};
}

struct VariancesTag {};

template <class... Ts> auto variances(Ts &&... ts) noexcept {
  return TaggedTuple<VariancesTag, Ts...>{
      {}, std::forward_as_tuple(std::forward<Ts>(ts)...)};
}

template <class T> auto variances(std::initializer_list<T> init) noexcept {
  using iter = typename std::initializer_list<T>::iterator;
  return TaggedTuple<VariancesTag, iter, iter>{
      {}, std::make_tuple(init.begin(), init.end())};
}

template <class... Args>
struct Values : decltype(values(std::declval<Args>()...)) {
  Values(Args &&... args)
      : decltype(values(std::declval<Args>()...))(
            core::values(std::forward<Args>(args)...)) {}
  template <class T>
  Values(std::initializer_list<T> init)
      : decltype(values(init))(core::values(init.begin(), init.end())) {}
};
template <class... Args> Values(Args &&... args)->Values<Args...>;
template <class T>
Values(std::initializer_list<T>)
    ->Values<typename std::initializer_list<T>::iterator,
             typename std::initializer_list<T>::iterator>;

template <class... Args>
struct Variances : decltype(variances(std::declval<Args>()...)) {
  Variances(Args &&... args)
      : decltype(variances(std::declval<Args>()...))(
            core::variances(std::forward<Args>(args)...)) {}
  template <class T>
  Variances(std::initializer_list<T> init)
      : decltype(variances(init))(core::variances(init.begin(), init.end())) {}
};
template <class... Args> Variances(Args &&... args)->Variances<Args...>;
template <class T>
Variances(std::initializer_list<T>)
    ->Variances<typename std::initializer_list<T>::iterator,
                typename std::initializer_list<T>::iterator>;

namespace detail {
template <class Tag, class T> struct has_tag : std::false_type {};

template <class Tag, class... Ts>
struct has_tag<Tag, TaggedTuple<Tag, Ts...>> : std::true_type {};

template <class Tag, class... Ts>
struct has_tag<Tag, Values<Ts...>> : std::is_same<Tag, ValuesTag> {};

template <class Tag, class... Ts>
struct has_tag<Tag, Variances<Ts...>> : std::is_same<Tag, VariancesTag> {};

template <class T, class... Args>
constexpr bool is_type_in_pack_v =
    std::disjunction<std::is_same<T, std::decay_t<Args>>...>::value;

template <class Tag, class... Args>
constexpr bool is_tag_in_pack_v =
    std::disjunction<has_tag<Tag, Args>...>::value;

template <class T, template <class T1, class T2> class Cond, class... Args>
struct Indexer {
  template <std::size_t... IS>
  static constexpr auto indexOfCorresponding_impl(std::index_sequence<IS...>) {
    return ((Cond<T, Args>::value * IS) + ...);
  }

  static constexpr auto indexOfCorresponding() {
    return indexOfCorresponding_impl(
        std::make_index_sequence<sizeof...(Args)>{});
  }
};

template <class T, class Tuple, std::size_t... I>
constexpr T make_move_from_tuple_impl(Tuple &&t, std::index_sequence<I...>) {
  return T(std::move(std::get<I>(t))...);
}

template <class T, class Tuple> constexpr T make_move_from_tuple(Tuple &&t) {
  return detail::make_move_from_tuple_impl<T>(
      std::forward<Tuple>(t),
      std::make_index_sequence<
          std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

template <class VarT, class ElemT, class... Ts>
class ConstructorArgumentsMatcher {
public:
  template <class... NonDataTypes> constexpr static void checkArgTypesValid() {
    constexpr int nonDataTypesCount =
        (is_type_in_pack_v<NonDataTypes, Ts...> + ...);
    constexpr bool hasVal = is_tag_in_pack_v<ValuesTag, Ts...>;
    constexpr bool hasVar = is_tag_in_pack_v<VariancesTag, Ts...>;
    static_assert(
        nonDataTypesCount + hasVal + hasVar == sizeof...(Ts),
        "Arguments: units::Unit, Shape, Dims, Values and Variences could only "
        "be used. Example: Variable(dtype<float>, units::Unit(units::kg), "
        "Shape{1, 2}, Dims{Dim::X, Dim::Y}, Values({3, 4}))");
  }

  template <class... NonDataTypes> static VarT construct(Ts &&... ts) {
    auto tp = std::make_tuple(std::forward<Ts>(ts)...);
    auto val = std::move(extractTagged<ValuesTag, Ts...>(tp).tuple);
    auto var = std::move(extractTagged<VariancesTag, Ts...>(tp).tuple);
    return VarT::template createVariable<ElemT>(
        std::forward<NonDataTypes>(extractArgs<NonDataTypes, Ts...>(tp))...,
        std::move(val), std::move(var));
  }

private:
  template <class T, class... Args>
  static decltype(auto) extractArgs(std::tuple<Args...> &tp) {
    if constexpr (!is_type_in_pack_v<T, Ts...>)
      return T{};
    else {
      constexpr auto index =
          Indexer<T, std::is_same, Args...>::indexOfCorresponding();
      return std::get<index>(tp);
    }
  }

  template <class Tag, class... Args>
  static decltype(auto) extractTagged(std::tuple<Args...> &tp) {
    if constexpr (!is_tag_in_pack_v<Tag, Ts...>)
      return TaggedTuple<Tag>();
    else {
      constexpr auto index =
          Indexer<Tag, has_tag, Args...>::indexOfCorresponding();
      return std::get<index>(tp);
    }
  }
};

} // namespace detail
} // namespace scipp::core

#endif // SCIPP_VALUES_AND_VARIANCES_STRONG_TYPES_H
