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
// The structs the OptionalContainer to treat the absence of argument in
// constructor. Values and Variances should be separate types (not aliases) to
// provide CTAD and custom deduction guides (because values and variables
// could be of different types) to simplify syntax.

template <class Tag, class... Ts> struct TaggedTuple {
  using tag_type = Tag;
  using tuple_type = std::tuple<Ts &&...>;
  Tag tag;
  std::tuple<Ts &&...> tuple;
};

struct ValuesTag {};

template <class... Ts> auto Values(Ts &&... ts) noexcept {
  return TaggedTuple<ValuesTag, Ts...>{
      {}, std::forward_as_tuple(std::forward<Ts>(ts)...)};
}

template <class T> auto Values(std::initializer_list<T> init) noexcept {
  return TaggedTuple<ValuesTag, Vector<T>>{
      {}, std::forward_as_tuple(Vector<T>(init)) /*init.begin(), init.end())*/};
}

struct VariancesTag {};

template <class... Ts> auto Variances(Ts &&... ts) noexcept {
  return TaggedTuple<VariancesTag, Ts...>{
      {}, std::forward_as_tuple(std::forward<Ts>(ts)...)};
}

template <class T> auto Variances(std::initializer_list<T> init) noexcept {
  return TaggedTuple<VariancesTag, Vector<T>>{
      {}, std::forward_as_tuple(Vector<T>(init)) /*init.begin(), init.end())*/};
}

namespace detail {
template <class Tag, class T> struct has_tag : std::false_type {};

template <class Tag, class... Ts>
struct has_tag<Tag, TaggedTuple<Tag, Ts...>> : std::true_type {};

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

template <class VarT, class ElemT, class... Ts>
class ConstructorArgumentsMatcher {
public:
  template <class... NonDataTypes> constexpr static void checkArgTypesValid() {
    constexpr int nonDataTypesCount =
        (is_type_in_pack_v<NonDataTypes, Ts...> + ...);
    constexpr bool hasVal = is_tag_in_pack_v<ValuesTag, Ts...>;
    constexpr bool hasVar = is_tag_in_pack_v<VariancesTag, Ts...>;
    static_assert(nonDataTypesCount + hasVal + hasVar == sizeof...(Ts));
  }

  template <class... NonDataTypes> static VarT construct(Ts &&... ts) {
    auto tp = std::make_tuple(std::forward<Ts>(ts)...);
    return VarT::template createVariable<ElemT>(
        std::forward<NonDataTypes>(extractArgs<NonDataTypes, Ts...>(tp))...,
        std::move(extractTagged<ValuesTag, Ts...>(tp).tuple),
        std::move(extractTagged<VariancesTag, Ts...>(tp).tuple));
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
