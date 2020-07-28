// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file Various transform functions for variables.
///
/// The underlying mechanism of the implementation is as follows:
/// 1. `visit` (or `visit_impl`) obtains the concrete underlying data type(s).
/// 2. `TransformInPlace` is applied to that concrete container, calling
///    `do_transform`. `TransformInPlace` essentially builds a callable
///    accepting a container from a callable accepting an element of the
///    container.
/// 3. `do_transform` is essentially a fancy std::transform. It provides
///    automatic handling of data that has variances in addition to values,
///    calling a different transform implementation for each case
///    (variants of transform_in_place_impl).
/// 4. The function implementing the transform calls the overloaded operator for
///    each element. Previously `TransformEvents` has been added to the overload
///    set of the operator and this will now correctly treat event data.
///    Essentially it causes a (single) recursive call to the transform
///    implementation (transform_in_place_impl). In this second call the
///    client-provided overload will match.
///
/// @author Simon Heybrock
#pragma once

#include "scipp/common/overloaded.h"

#include "scipp/core/parallel.h"
#include "scipp/core/transform_common.h"
#include "scipp/core/value_and_variance.h"
#include "scipp/core/values_and_variances.h"

#include "scipp/variable/except.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/visit.h"

namespace scipp::core::detail {
template <class T> struct element_type<ValuesAndVariances<event_list<T>>> {
  using type = T;
};
template <class T>
struct element_type<ValuesAndVariances<const event_list<T>>> {
  using type = T;
};
} // namespace scipp::core::detail

namespace scipp::variable {

namespace detail {

template <class T> struct has_variances : std::false_type {};
template <class T>
struct has_variances<ValueAndVariance<T>> : std::true_type {};
template <class T>
struct has_variances<ValuesAndVariances<T>> : std::true_type {};
template <class T>
inline constexpr bool has_variances_v = has_variances<T>::value;

/// Helper for the transform implementation to unify iteration of data with and
/// without variances as well as event_list and dense container.
template <class T>
static constexpr decltype(auto) value_maybe_variance(T &&range,
                                                     const scipp::index i) {
  if constexpr (has_variances_v<std::decay_t<T>>) {
    if constexpr (core::is_events_v<decltype(range.values.data()[0])>)
      return ValuesAndVariances{range.values.data()[i],
                                range.variances.data()[i]};
    else
      return ValueAndVariance{range.values.data()[i],
                              range.variances.data()[i]};
  } else {
    return range.data()[i];
  }
}

template <class T> struct is_eigen_type : std::false_type {};
template <class T, int Rows, int Cols>
struct is_eigen_type<Eigen::Matrix<T, Rows, Cols>> : std::true_type {};
template <class T, int Rows, int Cols>
struct is_eigen_type<event_list<Eigen::Matrix<T, Rows, Cols>>>
    : std::true_type {};
template <class T>
inline constexpr bool is_eigen_type_v = is_eigen_type<T>::value;

namespace transform_detail {
template <class T> struct is_events : std::false_type {};
template <class T> struct is_events<event_list<T>> : std::true_type {};
template <class T>
struct is_events<ValuesAndVariances<event_list<T>>> : std::true_type {};
template <class T>
struct is_events<ValuesAndVariances<const event_list<T>>> : std::true_type {};
template <class T> inline constexpr bool is_events_v = is_events<T>::value;
} // namespace transform_detail

template <class T> static auto check_and_get_size(const T &a) {
  return scipp::size(a);
}

template <class T1, class T2>
static auto check_and_get_size(const T1 &a, const T2 &b) {
  if constexpr (transform_detail::is_events_v<T1>) {
    if constexpr (transform_detail::is_events_v<T2>)
      core::expect::sizeMatches(a, b);
    return scipp::size(a);
  } else {
    return scipp::size(b);
  }
}

struct EventsFlag {};

// Helpers for handling a tuple of indices (integers or ViewIndex).
namespace iter {

template <class T, size_t... I>
static constexpr void increment_impl(T &&indices,
                                     std::index_sequence<I...>) noexcept {
  auto inc = [](auto &&i) {
    if constexpr (std::is_same_v<std::decay_t<decltype(i)>, core::ViewIndex>)
      i.increment();
    else
      ++i;
  };
  (inc(std::get<I>(indices)), ...);
}
template <class T> static constexpr void increment(T &indices) noexcept {
  increment_impl(indices, std::make_index_sequence<std::tuple_size_v<T>>{});
}

template <class T, size_t... I>
static constexpr void advance_impl(T &&indices, const scipp::index distance,
                                   std::index_sequence<I...>) noexcept {
  auto inc = [distance](auto &&i) {
    if constexpr (std::is_same_v<std::decay_t<decltype(i)>, core::ViewIndex>)
      i.setIndex(i.index() + distance);
    else
      i += distance;
  };
  (inc(std::get<I>(indices)), ...);
}
template <class T>
static constexpr void advance(T &indices,
                              const scipp::index distance) noexcept {
  advance_impl(indices, distance,
               std::make_index_sequence<std::tuple_size_v<T>>{});
}

template <class T> static constexpr auto begin_index(T &&iterable) noexcept {
  if constexpr (core::is_ElementArrayView_v<std::decay_t<T>>)
    return iterable.begin_index();
  else if constexpr (is_ValuesAndVariances_v<std::decay_t<T>>)
    return begin_index(iterable.values);
  else
    return scipp::index(0);
}

template <class T> static constexpr auto end_index(T &&iterable) noexcept {
  if constexpr (core::is_ElementArrayView_v<std::decay_t<T>>)
    return iterable.end_index();
  else if constexpr (is_ValuesAndVariances_v<std::decay_t<T>>)
    return end_index(iterable.values);
  else
    return scipp::size(iterable);
}

template <class T> static constexpr auto get(const T &index) noexcept {
  if constexpr (std::is_integral_v<T>)
    return index;
  else
    return index.get();
}

template <class T>
static constexpr auto has_stride_zero(const T &index) noexcept {
  if constexpr (std::is_integral_v<T>)
    return false;
  else
    return index.has_stride_zero();
}

} // namespace iter

template <class Op, class Indices, class... Args, size_t... I>
static constexpr auto call_impl(Op &&op, const Indices &indices,
                                std::index_sequence<I...>, Args &&... args) {
  return op(value_maybe_variance(args, iter::get(std::get<I + 1>(indices)))...);
}
template <class Op, class Indices, class Out, class... Args>
static constexpr void call(Op &&op, const Indices &indices, Out &&out,
                           Args &&... args) {
  const auto i = iter::get(std::get<0>(indices));
  auto &&out_ = value_maybe_variance(out, i);
  out_ = call_impl(std::forward<Op>(op), indices,
                   std::make_index_sequence<std::tuple_size_v<Indices> - 1>{},
                   std::forward<Args>(args)...);
  // If the output is events, ValuesAndVariances::operator= already does the job
  // in the line above (since ValuesAndVariances wraps references), if not
  // events then copy to actual output.
  if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(out_)>>) {
    out.values.data()[i] = out_.value;
    out.variances.data()[i] = out_.variance;
  }
}

template <class Op, class Indices, class Arg, class... Args, size_t... I>
static constexpr void call_in_place_impl(Op &&op, const Indices &indices,
                                         std::index_sequence<I...>, Arg &&arg,
                                         Args &&... args) {
  static_assert(
      std::is_same_v<
          decltype(op(arg, value_maybe_variance(
                               args, iter::get(std::get<I + 1>(indices)))...)),
          void>);
  op(arg, value_maybe_variance(args, iter::get(std::get<I + 1>(indices)))...);
}
template <class Op, class Indices, class Arg, class... Args>
static constexpr void call_in_place(Op &&op, const Indices &indices, Arg &&arg,
                                    Args &&... args) {
  const auto i = iter::get(std::get<0>(indices));
  // Two cases are distinguished here:
  // 1. In the case of event data we create ValuesAndVariances, which hold
  //    references that can be modified.
  // 2. For dense data we create ValueAndVariance, which performs an element
  //    copy, so the result has to be updated after the call to `op`.
  // Note that in the case of event data we actually have a recursive call to
  // transform_in_place_impl for the iteration over each individual
  // event_list. This then falls into case 2 and thus the recursion
  // terminates with the second level.
  auto &&arg_ = value_maybe_variance(arg, i);
  call_in_place_impl(std::forward<Op>(op), indices,
                     std::make_index_sequence<std::tuple_size_v<Indices> - 1>{},
                     std::forward<decltype(arg_)>(arg_),
                     std::forward<Args>(args)...);
  if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(arg_)>>) {
    arg.values.data()[i] = arg_.value;
    arg.variances.data()[i] = arg_.variance;
  }
}

template <class Op, class Out, class... Ts>
static void transform_elements(Op op, Out &&out, Ts &&... other) {
  auto run = [&](auto indices, const auto &end) {
    for (; std::get<0>(indices) != end; iter::increment(indices))
      call(op, indices, out, other...);
  };
  const auto begin =
      std::tuple{iter::begin_index(out), iter::begin_index(other)...};
  if constexpr (transform_detail::is_events_v<std::decay_t<Out>>) {
    run(begin, iter::end_index(out));
  } else {
    auto run_parallel = [&](const auto &range) {
      auto indices = begin;
      iter::advance(indices, range.begin());
      auto end = std::tuple{iter::begin_index(out)};
      iter::advance(end, range.end());
      run(indices, std::get<0>(end));
    };
    core::parallel::parallel_for(core::parallel::blocked_range(0, out.size()),
                                 run_parallel);
  }
}

/// Broadcast a constant to arbitrary size. Helper for TransformEvents.
///
/// This helper allows the use of a common transform implementation when mixing
/// events and non-event data.
template <class T> struct broadcast {
  using value_type = std::remove_const_t<T>;
  constexpr auto operator[](const scipp::index) const noexcept { return value; }
  constexpr auto data() const noexcept { return *this; }
  T value;
};
template <class T> broadcast(T) -> broadcast<T>;

template <class T> static decltype(auto) maybe_broadcast(T &&value) {
  if constexpr (transform_detail::is_events_v<std::decay_t<T>>)
    return std::forward<T>(value);
  else
    return broadcast{value};
}

template <class T> struct is_broadcast : std::false_type {};
template <class T> struct is_broadcast<broadcast<T>> : std::true_type {};
template <class T>
inline constexpr bool is_broadcast_v = is_broadcast<T>::value;

template <class T>
struct is_eigen_expression
    : std::is_base_of<Eigen::MatrixBase<std::decay_t<T>>, std::decay_t<T>> {};

template <class T> static constexpr auto maybe_eval(T &&_) {
  if constexpr (is_eigen_expression<T>::value)
    return _.eval();
  else
    return std::forward<T>(_);
}

/// Functor for implementing operations with event data, see also
/// TransformEventsInPlace.
struct TransformEvents {
  template <class Op, class... Ts>
  constexpr auto operator()(const Op &op, const Ts &... args) const {
    event_list<std::invoke_result_t<Op, core::detail::element_type_t<Ts>...>>
        vals(check_and_get_size(args...));
    if constexpr (!std::is_base_of_v<core::transform_flags::no_out_variance_t,
                                     Op> &&
                  (has_variances_v<Ts> || ...)) {
      auto vars(vals);
      ValuesAndVariances out{vals, vars};
      transform_elements(op, out, maybe_broadcast(args)...);
      return std::pair(std::move(vals), std::move(vars));
    } else {
      transform_elements(op, vals, maybe_broadcast(args)...);
      return vals;
    }
  }
};

template <class Op, class... Args>
constexpr bool check_all_or_none_variances =
    std::is_base_of_v<core::transform_flags::expect_all_or_none_have_variance_t,
                      Op> &&
    !std::conjunction_v<is_ValuesAndVariances<std::decay_t<Args>>...> &&
    std::disjunction_v<is_ValuesAndVariances<std::decay_t<Args>>...>;

/// Recursion endpoint for do_transform.
///
/// Call transform_elements with or without variances for output, depending on
/// whether any of the arguments has variances or not.
template <class Op, class Out, class Tuple>
static void do_transform(Op op, Out &&out, Tuple &&processed) {
  auto out_val = out.values();
  std::apply(
      [&op, &out, &out_val](auto &&... args) {
        if constexpr (check_all_or_none_variances<Op, decltype(args)...>) {
          throw except::VariancesError(
              "Expected either all or none of inputs to have variances.");
        } else if constexpr (!std::is_base_of_v<
                                 core::transform_flags::no_out_variance_t,
                                 Op> &&
                             (is_ValuesAndVariances_v<
                                  std::decay_t<std::decay_t<decltype(args)>>> ||
                              ...)) {
          auto out_var = out.variances();
          transform_elements(op, ValuesAndVariances{out_val, out_var},
                             std::forward<decltype(args)>(args)...);
        } else {
          transform_elements(op, out_val,
                             std::forward<decltype(args)>(args)...);
        }
      },
      std::forward<Tuple>(processed));
}

/// Helper for transform implementation, performing branching between output
/// with and without variances as well as handling other operands with and
/// without variances.
template <class Op, class Out, class Tuple, class Arg, class... Args>
static void do_transform(Op op, Out &&out, Tuple &&processed, const Arg &arg,
                         const Args &... args) {
  auto vals = arg.values();
  if (arg.hasVariances()) {
    if constexpr (std::is_base_of_v<
                      core::transform_flags::expect_no_variance_arg_t<
                          std::tuple_size_v<Tuple>>,
                      Op>) {
      throw except::VariancesError("Variances in argument " +
                                   std::to_string(std::tuple_size_v<Tuple>) +
                                   " not supported.");
    } else if constexpr (core::canHaveVariances<typename Arg::value_type>()) {
      auto vars = arg.variances();
      do_transform(
          op, std::forward<Out>(out),
          std::tuple_cat(processed, std::tuple(ValuesAndVariances{vals, vars})),
          args...);
    }
  } else {
    if constexpr (std::is_base_of_v<
                      core::transform_flags::expect_variance_arg_t<
                          std::tuple_size_v<Tuple>>,
                      Op>)
      throw except::VariancesError("Variances missing in argument " +
                                   std::to_string(std::tuple_size_v<Tuple>) +
                                   " . Must be set.");
    do_transform(op, std::forward<Out>(out),
                 std::tuple_cat(processed, std::tuple(vals)), args...);
  }
}

template <class T> struct as_view {
  using value_type = typename T::value_type;
  bool hasVariances() const { return data.hasVariances(); }
  auto values() const { return data.valuesView(dims); }
  auto variances() const { return data.variancesView(dims); }

  T &data;
  const Dimensions &dims;
};
template <class T> as_view(T &data, const Dimensions &dims) -> as_view<T>;

template <class Op> struct Transform {
  Op op;
  template <class... Ts> Variable operator()(Ts &&... handles) const {
    const auto dims = merge(handles.dims()...);
    using Out = decltype(maybe_eval(op(handles.values()[0]...)));
    constexpr bool out_variances =
        !std::is_base_of_v<core::transform_flags::no_out_variance_t, Op>;
    auto volume = dims.volume();
    Variable out =
        out_variances && (handles.hasVariances() || ...)
            ? makeVariable<Out>(Dimensions{dims},
                                Values(volume, core::default_init_elements),
                                Variances(volume, core::default_init_elements))
            : makeVariable<Out>(Dimensions{dims},
                                Values(volume, core::default_init_elements));
    auto &outT = static_cast<VariableConceptT<Out> &>(out.data());
    do_transform(op, outT, std::tuple<>(), as_view{handles, dims}...);
    return out;
  }
};
template <class Op> Transform(Op) -> Transform<Op>;

template <class T, class Handle> struct optional_events;
template <class T, class... Known>
struct optional_events<T, std::tuple<Known...>> {
  using type = std::conditional_t<std::disjunction_v<std::is_same<T, Known>...>,
                                  std::tuple<T>, std::tuple<>>;
};

// std::tuple_cat does not work correctly on with clang-7. Issue with
// Eigen::Vector3d.
template <typename T, typename...> struct tuple_cat { using type = T; };
template <template <typename...> class C, typename... Ts1, typename... Ts2,
          typename... Ts3>
struct tuple_cat<C<Ts1...>, C<Ts2...>, Ts3...>
    : public tuple_cat<C<Ts1..., Ts2...>, Ts3...> {};
template <class... Ts> using tuple_cat_t = typename tuple_cat<Ts...>::type;

template <class T1, class T2, class Handle> struct optional_events_pair;
template <class T1, class T2, class... Known>
struct optional_events_pair<T1, T2, std::tuple<Known...>> {
  using type =
      std::conditional_t<std::disjunction_v<std::is_same<T1, Known>...> &&
                             std::disjunction_v<std::is_same<T2, Known>...>,
                         std::tuple<std::tuple<T1, T2>>, std::tuple<>>;
};
using supported_event_types =
    std::tuple<double, float, int64_t, int32_t, bool, event_list<double>,
               event_list<float>, event_list<int64_t>, event_list<int32_t>,
               event_list<bool>>;
template <class T>
using optional_events_t =
    typename optional_events<T, supported_event_types>::type;
template <class T1, class T2>
using optional_events_pair_t =
    typename optional_events_pair<T1, T2, supported_event_types>::type;

/// Augment a tuple of types with the corresponding events types, if they exist.
struct augment {
  template <class... Ts> static auto insert_events(const std::tuple<Ts...> &) {
    return tuple_cat_t<std::tuple<Ts...>,
                       optional_events_t<event_list<Ts>>...>{};
  }

  template <class... Ts>
  static auto insert_events_in_place(const std::tuple<Ts...> &tuple) {
    return insert_events(tuple);
  }

  template <class... T>
  static auto insert_events_in_place(const std::tuple<std::tuple<T>...> &) {
    return tuple_cat_t<std::tuple<std::tuple<T>...>,
                       optional_events_t<event_list<T>>...>{};
  }
  template <class... First, class... Second>
  static auto
  insert_events_in_place(const std::tuple<std::tuple<First, Second>...> &) {
    return tuple_cat_t<
        std::tuple<std::tuple<First, Second>...>,
        optional_events_pair_t<event_list<First>, Second>...,
        optional_events_pair_t<event_list<First>, event_list<Second>>...>{};
  }
  template <class... T>
  static auto insert_events(const std::tuple<std::tuple<T>...> &) {
    return tuple_cat_t<std::tuple<std::tuple<T>...>,
                       optional_events_t<event_list<T>>...>{};
  }
  template <class... First, class... Second>
  static auto insert_events(const std::tuple<std::tuple<First, Second>...> &) {
    return tuple_cat_t<
        std::tuple<std::tuple<First, Second>...>,
        optional_events_pair_t<First, event_list<Second>>...,
        optional_events_pair_t<event_list<First>, Second>...,
        optional_events_pair_t<event_list<First>, event_list<Second>>...>{};
  }
};

template <class Op, class EventsOp> struct overloaded_events : Op, EventsOp {
  template <class... Ts> constexpr auto operator()(Ts &&... args) const {
    if constexpr ((transform_detail::is_events_v<std::decay_t<Ts>> || ...))
      return EventsOp::operator()(static_cast<const Op &>(*this),
                                  std::forward<Ts>(args)...);
    else if constexpr ((is_eigen_type_v<std::decay_t<Ts>> || ...))
      // WARNING! The explicit specification of the template arguments of
      // operator() is EXTREMELY IMPORTANT. It ensures that Eigen types are
      // passed BY REFERENCE and NOT BY VALUE. Passing by value leads to
      // construction of expressions of values on the stack, which are then
      // returned from the operator. One way to identify this is using
      // address-sanitizer, which finds a `stack-use-after-scope`.
      return Op::template operator()<Ts...>(std::forward<Ts>(args)...);
    else
      return Op::template operator()(std::forward<Ts>(args)...);
  }
};
template <class... Ts> overloaded_events(Ts...) -> overloaded_events<Ts...>;

template <class T>
struct is_any_events : std::conditional_t<core::is_events<T>::value,
                                          std::true_type, std::false_type> {};
template <class... Ts>
struct is_any_events<std::tuple<Ts...>>
    : std::conditional_t<(core::is_events<Ts>::value || ...), std::true_type,
                         std::false_type> {};

} // namespace detail

template <class... Ts, class Op>
static constexpr auto type_tuples(Op) noexcept {
  if constexpr (sizeof...(Ts) == 0)
    return typename Op::types{};
  else if constexpr ((visit_detail::is_tuple<Ts>::value || ...))
    return typename detail::tuple_cat<Ts...>::type{};
  else
    return std::tuple<Ts...>{};
}

/// Helper class wrapping functions for in-place transform.
///
/// The dry_run template argument can be used to disable any actual modification
/// of data. This is used to implement operations on datasets with a strong
/// exception guarantee.
template <bool dry_run> struct in_place {
  template <class Op, class T, class... Ts>
  static void transform_in_place_impl(Op op, T &&arg, Ts &&... other) {
    using namespace detail;
    const auto begin =
        std::tuple{iter::begin_index(arg), iter::begin_index(other)...};
    // For event data we can fail for any subitem if the sizes to not match.
    // To avoid partially modifying (and thus corrupting) data in an in-place
    // operation we need to do the checks before any modification happens.
    if constexpr (core::is_events_v<typename std::decay_t<T>::value_type> ||
                  (core::is_events_v<typename std::decay_t<Ts>::value_type> ||
                   ...)) {
      const auto end = iter::end_index(arg);
      for (auto i = begin; std::get<0>(i) != end; iter::increment(i)) {
        call_in_place(
            [](auto &&... args) {
              if constexpr (std::is_base_of_v<EventsFlag, Op>)
                static_cast<void>(check_and_get_size(args...));
            },
            i, arg, other...);
      }
    }
    if constexpr (dry_run)
      return;
    auto run = [&](auto indices, const auto &end) {
      for (; std::get<0>(indices) != end; iter::increment(indices))
        call_in_place(op, indices, arg, other...);
    };

    if constexpr (transform_detail::is_events_v<std::decay_t<T>> ||
                  (transform_detail::is_events_v<std::decay_t<Ts>> || ...)) {
      run(begin, iter::end_index(arg));
    } else {
      if (iter::has_stride_zero(std::get<0>(begin))) {
        // The output has a dimension with stride zero so parallelization must
        // be done differently. Explicit and precise control of chunking is
        // required to avoid multiple threads writing to the same output. Not
        // implemented for now.
        run(begin, iter::end_index(arg));
      } else {
        auto run_parallel = [&](const auto &range) {
          auto indices = begin;
          iter::advance(indices, range.begin());
          auto end = std::tuple{iter::begin_index(arg)};
          iter::advance(end, range.end());
          run(indices, std::get<0>(end));
        };
        core::parallel::parallel_for(
            core::parallel::blocked_range(0, arg.size()), run_parallel);
      }
    }
  }

  /// Recursion endpoint for do_transform_in_place.
  ///
  /// Calls transform_in_place_impl unless the output has no variance even
  /// though it should.
  template <class Op, class Tuple>
  static void do_transform_in_place(Op op, Tuple &&processed) {
    using namespace detail;
    std::apply(
        [&op](auto &&arg, auto &&... args) {
          if constexpr (check_all_or_none_variances<Op, decltype(arg),
                                                    decltype(args)...>) {
            throw except::VariancesError(
                "Expected either all or none of inputs to have variances.");
          } else {
            constexpr bool in_var_if_out_var = std::is_base_of_v<
                core::transform_flags::expect_in_variance_if_out_variance_t,
                Op>;
            constexpr bool arg_var =
                is_ValuesAndVariances_v<std::decay_t<decltype(arg)>>;
            constexpr bool args_var =
                (is_ValuesAndVariances_v<std::decay_t<decltype(args)>> || ...);
            if constexpr ((in_var_if_out_var ? arg_var == args_var
                                             : arg_var || !args_var) ||
                          std::is_base_of_v<core::transform_flags::
                                                expect_no_variance_arg_t<0>,
                                            Op>) {
              transform_in_place_impl(op, std::forward<decltype(arg)>(arg),
                                      std::forward<decltype(args)>(args)...);
            } else {
              throw except::VariancesError(
                  "Output has no variance but at least one input does.");
            }
          }
        },
        std::forward<Tuple>(processed));
  }

  /// Helper for in-place transform implementation, performing branching between
  /// output with and without variances as well as handling other operands with
  /// and without variances.
  template <class Op, class Tuple, class Arg, class... Args>
  static void do_transform_in_place(Op op, Tuple &&processed, Arg &arg,
                                    const Args &... args) {
    using namespace detail;
    auto vals = arg.values();
    if (arg.hasVariances()) {
      if constexpr (std::is_base_of_v<
                        core::transform_flags::expect_no_variance_arg_t<
                            std::tuple_size_v<Tuple>>,
                        Op>) {
        throw except::VariancesError("Variances in argument " +
                                     std::to_string(std::tuple_size_v<Tuple>) +
                                     " not supported.");
      } else if constexpr (core::canHaveVariances<typename Arg::value_type>()) {
        auto vars = arg.variances();
        do_transform_in_place(
            op,
            std::tuple_cat(processed,
                           std::tuple(ValuesAndVariances{vals, vars})),
            args...);
      }
    } else {
      if constexpr (std::is_base_of_v<
                        core::transform_flags::expect_variance_arg_t<
                            std::tuple_size_v<Tuple>>,
                        Op>) {
        throw except::VariancesError("Argument " +
                                     std::to_string(std::tuple_size_v<Tuple>) +
                                     " must have variances.");
      } else {
        do_transform_in_place(op, std::tuple_cat(processed, std::tuple(vals)),
                              args...);
      }
    }
  }

  /// Functor for implementing in-place operations with event data.
  ///
  /// This is (conditionally) added to an overloaded set of operators provided
  /// by the user. If the data is events the overloads by this functor will
  /// match in place of the user-provided ones. We then recursively call the
  /// transform function. In this second call we have descended into the events
  /// container so now the user-provided overload will match directly.
  struct TransformEventsInPlace : public detail::EventsFlag {
    template <class Op, class... Ts>
    constexpr void operator()(const Op &op, Ts &&... args) const {
      using namespace detail;
      static_cast<void>(check_and_get_size(args...));
      transform_in_place_impl(op, maybe_broadcast(args)...);
    }
  };

  /// Functor for in-place transformation, applying `op` to all elements.
  ///
  /// This is responsible for converting the client-provided functor `Op` which
  /// operates on elements to a functor for the data container, which is
  /// required by `visit`.
  template <class Op> struct TransformInPlace {
    Op op;
    template <class T> void operator()(T &&handle) const {
      using namespace detail;
      auto view = as_view{handle, handle.dims()};
      if (handle.isContiguous())
        do_transform_in_place(op, std::tuple<>{}, handle);
      else
        do_transform_in_place(op, std::tuple<>{}, view);
    }

    template <class A, class B> void operator()(A &&a, B &&b) const {
      using namespace detail;
      const auto &dimsA = a.dims();
      const auto &dimsB = b.dims();
      if constexpr (std::is_same_v<typename std::remove_reference_t<decltype(
                                       a)>::value_type,
                                   typename std::remove_reference_t<decltype(
                                       b)>::value_type>) {
        if (a.valuesView(dimsA).overlaps(b.valuesView(dimsA))) {
          // If there is an overlap between lhs and rhs we copy the rhs before
          // applying the operation.
          const auto &b_ = b.copyT();
          // Ensuring that we call exactly same instance of operator() to avoid
          // extra template instantiations, do not remove the static_cast.
          return operator()(std::forward<A>(a), static_cast<B>(*b_));
        }
      }

      if (a.isContiguous() && dimsA.contains(dimsB)) {
        if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
          do_transform_in_place(op, std::tuple<>{}, a, b);
        } else {
          do_transform_in_place(op, std::tuple<>{}, a, as_view{b, dimsA});
        }
      } else {
        // If LHS has fewer dimensions than RHS, e.g., for computing sum the
        // view for iteration is based on dimsB.
        const auto viewDims = dimsA.contains(dimsB) ? dimsA : dimsB;
        auto a_view = as_view{a, viewDims};
        if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
          do_transform_in_place(op, std::tuple<>{}, a_view, b);
        } else {
          do_transform_in_place(op, std::tuple<>{}, a_view,
                                as_view{b, viewDims});
        }
      }
    }

    template <class T, class... Ts>
    void operator()(T &&out, Ts &&... handles) const {
      using namespace detail;
      const auto dims = merge(out.dims(), handles.dims()...);
      auto out_view = as_view{out, dims};
      do_transform_in_place(op, std::tuple<>{}, out_view,
                            as_view{handles, dims}...);
    }
  };
  // gcc cannot deal with deduction guide for nested class => helper function.
  template <class Op> static auto makeTransformInPlace(Op op) {
    return TransformInPlace<Op>{op};
  }

  template <class... Ts, class Op, class Var, class... Other>
  static void transform_data(std::tuple<Ts...> &&, Op op, Var &&var,
                             const Other &... other) {
    using namespace detail;
    try {
      // If a event_list<T> is specified explicitly as a type we assume
      // that the caller provides a matching overload. Otherwise we assume the
      // provided operator is for individual elements (regardless of whether
      // they are elements of dense or event data), so we add overloads for
      // event data processing.
      if constexpr ((is_any_events<Ts>::value || ...)) {
        visit_impl<Ts...>::apply(makeTransformInPlace(op), var.data(),
                                 other.data()...);
      } else if constexpr (sizeof...(Other) > 1) {
        // No event data supported yet in this case.
        variable::visit(std::tuple<Ts...>{})
            .apply(makeTransformInPlace(op), var.data(), other.data()...);
      } else {
        // Note that if only one of the inputs is events it must be the one
        // being transformed in-place, so there are only three cases here.
        variable::visit(
            augment::insert_events_in_place(
                std::tuple<
                    visit_detail::maybe_duplicate<Ts, Var, Other...>...>{}))
            .apply(makeTransformInPlace(
                       overloaded_events{op, TransformEventsInPlace{}}),
                   var.data(), other.data()...);
      }
    } catch (const std::bad_variant_access &) {
      throw except::TypeError("Cannot apply operation to item dtypes ", var,
                              other...);
    }
  }
  template <class... Ts, class Op, class Var, class... Other>
  static void transform(Op op, Var &&var, const Other &... other) {
    using namespace detail;
    (scipp::expect::contains(var.dims(), other.dims()), ...);
    auto unit = var.unit();
    op(unit, other.unit()...);
    // Stop early in bad cases of changing units (if `var` is a slice):
    var.expectCanSetUnit(unit);
    // Wrapped implementation to convert multiple tuples into a parameter pack.
    transform_data(type_tuples<Ts...>(op), op, std::forward<Var>(var),
                   other...);
    if constexpr (dry_run)
      return;
    var.setUnit(unit);
  }
};

/// Transform the data elements of a variable in-place.
///
/// Note that this is deliberately not named `for_each`: Unlike std::for_each,
/// this function does not promise in-order execution. This overload is
/// equivalent to std::transform with a single input range and an output range
/// identical to the input range, but avoids potentially costly element copies.
template <class... Ts, class Var, class Op>
void transform_in_place(Var &&var, Op op) {
  in_place<false>::transform<Ts...>(op, std::forward<Var>(var));
}

/// Transform the data elements of a variable in-place.
///
/// This overload is equivalent to std::transform with two input ranges and an
/// output range identical to the secound input range, but avoids potentially
/// costly element copies.
template <class... TypePairs, class Var, class Op>
void transform_in_place(Var &&var, const VariableConstView &other, Op op) {
  in_place<false>::transform<TypePairs...>(op, std::forward<Var>(var), other);
}

/// Transform the data elements of a variable in-place.
template <class... TypePairs, class Var, class Op>
void transform_in_place(Var &&var, const VariableConstView &var1,
                        const VariableConstView &var2, Op op) {
  in_place<false>::transform<TypePairs...>(op, std::forward<Var>(var), var1,
                                           var2);
}

/// Accumulate data elements of a variable in-place.
///
/// This is equivalent to `transform_in_place`, with the only difference that
/// the dimension check of the inputs is reversed. That is, it must be possible
/// to broadcast the dimension of the first argument to that of the other
/// argument. As a consequence, the operation may be applied multiple times to
/// the same output element, effectively accumulating the result.
///
/// WARNING: In contrast to the transform algorithms, accumulate does not touch
/// the unit, since it would be hard to track, e.g., in multiplication
/// operations.
template <class... TypePairs, class Var, class Op>
void accumulate_in_place(Var &&var, const VariableConstView &other, Op op) {
  scipp::expect::contains(other.dims(), var.dims());
  // Wrapped implementation to convert multiple tuples into a parameter pack.
  in_place<false>::transform_data(type_tuples<TypePairs...>(op), op,
                                  std::forward<Var>(var), other);
}
template <class... TypePairs, class Var, class Op>
void accumulate_in_place(Var &&var, const VariableConstView &var1,
                         const VariableConstView &var2, Op op) {
  scipp::expect::contains(var1.dims(), var.dims());
  scipp::expect::contains(var2.dims(), var.dims());
  in_place<false>::transform_data(type_tuples<TypePairs...>(op), op,
                                  std::forward<Var>(var), var1, var2);
}

namespace dry_run {
template <class... Ts, class Var, class Op>
void transform_in_place(Var &&var, Op op) {
  in_place<true>::transform<Ts...>(op, std::forward<Var>(var));
}
template <class... TypePairs, class Var, class Op>
void transform_in_place(Var &&var, const VariableConstView &other, Op op) {
  in_place<true>::transform<TypePairs...>(op, std::forward<Var>(var), other);
}
} // namespace dry_run

namespace detail {
template <class... Ts, class Op, class... Vars>
Variable transform(std::tuple<Ts...> &&, Op op, const Vars &... vars) {
  using namespace detail;
  auto unit = op(vars.unit()...);
  Variable out;
  try {
    if constexpr ((is_any_events<Ts>::value || ...) || sizeof...(Vars) > 2) {
      out = visit_impl<Ts...>::apply(Transform{op}, vars.data()...);
    } else {
      out =
          variable::visit(
              augment::insert_events(
                  std::tuple<visit_detail::maybe_duplicate<Ts, Vars...>...>{}))
              .apply(Transform{overloaded_events{op, TransformEvents{}}},
                     vars.data()...);
    }
  } catch (const std::bad_variant_access &) {
    throw except::TypeError("Cannot apply operation to item dtypes ", vars...);
  }
  out.setUnit(unit);
  return out;
}
} // namespace detail

/// Transform the data elements of a variable and return a new Variable.
///
/// This overload is equivalent to std::transform with a single input range, but
/// avoids the need to manually create a new variable for the output and the
/// need for, e.g., std::back_inserter.
template <class... Ts, class Op>
[[nodiscard]] Variable transform(const VariableConstView &var, Op op) {
  return detail::transform(type_tuples<Ts...>(op), op, var);
}

/// Transform the data elements of two variables and return a new Variable.
///
/// This overload is equivalent to std::transform with two input ranges, but
/// avoids the need to manually create a new variable for the output and the
/// need for, e.g., std::back_inserter.
template <class... Ts, class Op>
[[nodiscard]] Variable transform(const VariableConstView &var1,
                                 const VariableConstView &var2, Op op) {
  return detail::transform(type_tuples<Ts...>(op), op, var1, var2);
}

/// Transform the data elements of three variables and return a new Variable.
template <class... Ts, class Op>
[[nodiscard]] Variable transform(const VariableConstView &var1,
                                 const VariableConstView &var2,
                                 const VariableConstView &var3, Op op) {
  return detail::transform(type_tuples<Ts...>(op), op, var1, var2, var3);
}

/// Transform the data elements of four variables and return a new Variable.
template <class... Ts, class Op>
[[nodiscard]] Variable
transform(const VariableConstView &var1, const VariableConstView &var2,
          const VariableConstView &var3, const VariableConstView &var4, Op op) {
  return detail::transform(type_tuples<Ts...>(op), op, var1, var2, var3, var4);
}

} // namespace scipp::variable
