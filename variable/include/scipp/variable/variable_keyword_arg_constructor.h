// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <type_traits>

namespace scipp::variable {

namespace detail {
template <class U> struct vector {
  std::vector<U> data;
  template <class... Args>
  vector(Args &&... args) : data(std::forward<Args>(args)...) {}
  template <class A, class B> // avoid use of vector(size, value)
  vector(A &&a, B &&b) : data(std::initializer_list<U>{a, b}) {}
  template <class T>
  vector(std::initializer_list<T> init) : data(init.begin(), init.end()) {}
};

template <template <class...> class Derived, class... Args> struct arg_tuple {
  std::tuple<std::decay_t<Args>...> tuple;
  arg_tuple(Args &&... args) : tuple(std::forward<Args>(args)...) {}
};
} // namespace detail

using Shape = detail::vector<scipp::index>;
using Dims = detail::vector<Dim>;

template <class... Args>
struct Values : public detail::arg_tuple<Values, Args...> {
  using detail::arg_tuple<Values, Args...>::arg_tuple;
  template <class T>
  Values(std::initializer_list<T> init)
      : detail::arg_tuple<Values, Args...>(std::move(init)) {}
};
template <class... Args>
struct Variances : public detail::arg_tuple<Variances, Args...> {
  using detail::arg_tuple<Variances, Args...>::arg_tuple;
  template <class T>
  Variances(std::initializer_list<T> init)
      : detail::arg_tuple<Variances, Args...>(std::move(init)) {}
};

template <class... Args> Values(Args &&... args) -> Values<Args...>;
template <class T>
Values(std::initializer_list<T>) -> Values<std::initializer_list<T>>;

template <class... Args> Variances(Args &&... args) -> Variances<Args...>;
template <class T>
Variances(std::initializer_list<T>) -> Variances<std::initializer_list<T>>;

namespace detail {

void throw_keyword_arg_constructor_bad_dtype(const DType dtype);

/// Convert "keyword" args to tuple that can be used to construct Variable
///
/// This is an implementation detail of `makeVariable`.
template <class ElemT> struct ArgParser {
  std::tuple<units::Unit, Dimensions, element_array<ElemT>,
             std::optional<element_array<ElemT>>>
      args;
  Dims dims;
  Shape shape;

  void parse(const units::Unit &arg) { std::get<units::Unit>(args) = arg; }

  void parse(const Dimensions &arg) { std::get<Dimensions>(args) = arg; }

  void parse(const Dims &arg) {
    if (shape.data.empty())
      dims = arg;
    else
      std::get<Dimensions>(args) = Dimensions(arg.data, shape.data);
  }

  void parse(const Shape &arg) {
    if (dims.data.empty())
      shape = arg;
    else
      std::get<Dimensions>(args) = Dimensions(dims.data, arg.data);
  }

  template <class... Args> void parse(Values<Args...> &&arg) {
    if constexpr (std::is_constructible_v<element_array<ElemT>, Args...>)
      std::get<2>(args) =
          std::make_from_tuple<element_array<ElemT>>(std::move(arg.tuple));
    else
      throw_keyword_arg_constructor_bad_dtype(core::dtype<ElemT>);
  }

  template <class... Args> void parse(Variances<Args...> &&arg) {
    if constexpr (std::is_constructible_v<element_array<ElemT>, Args...>)
      std::get<3>(args) =
          std::make_from_tuple<element_array<ElemT>>(std::move(arg.tuple));
    else
      throw_keyword_arg_constructor_bad_dtype(core::dtype<ElemT>);
  }
};

} // namespace detail
} // namespace scipp::variable
