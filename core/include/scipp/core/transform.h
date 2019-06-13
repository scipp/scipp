// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
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
///    (transform_in_place_impl or transform_in_place_with_variance_impl).
/// 4. The function implementing the transform calls the overloaded operator for
///    each element. Previously `TransformSparse` has been added to the overload
///    set of the operator and this will now correctly treat sparse data.
///    Essentially it causes a (single) recursive call to the transform
///    implementation (transform_in_place_impl or
///    transform_in_place_with_variance_impl). In this second call the
///    client-provided overload will match.
///
/// @author Simon Heybrock
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "scipp/core/except.h"
#include "scipp/core/variable.h"
#include "scipp/core/visit.h"

namespace scipp::core {

namespace detail {

/// A value/variance pair with operators that propagate uncertainties.
///
/// This is intended for small T such as double, float, and int. It is the
/// central implementation of uncertainty propagation in scipp, for built-in
/// operations as well as custom operations using one of the transform
/// functions. Since T is assumed to be small it is copied into the class and
/// extracted later. See also ValuesAndVariances.
template <class T> struct ValueAndVariance {
  T value;
  T variance;

  template <class T2>
  constexpr auto &operator=(const ValueAndVariance<T2> other) noexcept {
    value = other.value;
    variance = other.variance;
    return *this;
  }

  template <class T2> constexpr auto &operator+=(const T2 other) noexcept {
    return *this = *this + other;
  }
  template <class T2> constexpr auto &operator-=(const T2 other) noexcept {
    return *this = *this - other;
  }
  template <class T2> constexpr auto &operator*=(const T2 other) noexcept {
    return *this = *this * other;
  }
  template <class T2> constexpr auto &operator/=(const T2 other) noexcept {
    return *this = *this / other;
  }
};

template <class T>
constexpr auto operator-(const ValueAndVariance<T> a) noexcept {
  return ValueAndVariance{-a.value, a.variance};
}

template <class T> constexpr auto sqrt(const ValueAndVariance<T> a) noexcept {
  using std::sqrt;
  return ValueAndVariance{sqrt(a.value), 0.25 * (a.variance / a.value)};
}

template <class T> constexpr auto abs(const ValueAndVariance<T> a) noexcept {
  using std::abs;
  return ValueAndVariance{abs(a.value), a.variance};
}

template <class T1, class T2>
constexpr auto operator+(const ValueAndVariance<T1> a,
                         const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a.value + b.value, a.variance + b.variance};
}
template <class T1, class T2>
constexpr auto operator-(const ValueAndVariance<T1> a,
                         const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a.value - b.value, a.variance - b.variance};
}
template <class T1, class T2>
constexpr auto operator*(const ValueAndVariance<T1> a,
                         const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a.value * b.value,
                          a.variance * b.value * b.value +
                              b.variance * a.value * a.value};
}
template <class T1, class T2>
constexpr auto operator/(const ValueAndVariance<T1> a,
                         const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{
      a.value / b.value,
      (a.variance + b.variance * (a.value * a.value) / (b.value * b.value)) /
          (b.value * b.value)};
}

template <class T1, class T2>
constexpr auto operator+(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value + b, a.variance};
}
template <class T1, class T2>
constexpr auto operator+(const T2 a, const ValueAndVariance<T1> b) noexcept {
  return ValueAndVariance{a + b.value, b.variance};
}
template <class T1, class T2>
constexpr auto operator-(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value - b, a.variance};
}
template <class T1, class T2>
constexpr auto operator-(const T2 a, const ValueAndVariance<T1> b) noexcept {
  return ValueAndVariance{a - b.value, b.variance};
}
template <class T1, class T2>
constexpr auto operator*(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value * b, a.variance * b * b};
}
template <class T1, class T2>
constexpr auto operator*(const T1 a, const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a * b.value, a * a * b.variance};
}
template <class T1, class T2>
constexpr auto operator/(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value / b, a.variance / (b * b)};
}
template <class T1, class T2>
constexpr auto operator/(const T1 a, const ValueAndVariance<T2> b) noexcept {
  return ValueAndVariance{a / b.value, b.variance * a * a /
                                           (b.value * b.value) /
                                           (b.value * b.value)};
}

/// Deduction guide for class ValueAndVariances. Using decltype to deal with
/// potential mixed-type val and var arguments arising in binary operations
/// between, e.g., double and float.
template <class T1, class T2>
ValueAndVariance(const T1 &val, const T2 &var)
    ->ValueAndVariance<decltype(T1() + T2())>;

/// A values/variances pair based on references to sparse data containers.
///
/// This is a helper for implementing operations for sparse container such as
/// `clear`, and for descending into the sparse container itself, using a nested
/// call to an iteration function.
template <class T> struct ValuesAndVariances {
  ValuesAndVariances(T &val, T &var) : values(val), variances(var) {
    expect::sizeMatches(values, variances);
  }
  T &values;
  T &variances;

  void clear() {
    values.clear();
    variances.clear();
  }

  // TODO Note that methods like insert, begin, and end are required as long as
  // we support sparse data via a plain container such as std::vector, e.g., for
  // concatenation using a.insert(a.end(), b.begin(), b.end()). Instead of
  // adding these methods here (essentially making ValuesAndVariances support a
  // proxy iterator) it might be easier to wrap the sparse container and give it
  // some functionality like concatenate. Then we simply need to support this
  // method here, which is much simpler than having proxy iterators.
  template <class... Ts> void insert(Ts &&...) {
    throw std::runtime_error(
        "`insert` not implemented for sparse data with variances.");
  }

  void *begin() {
    throw std::runtime_error(
        "`begin` not implemented for sparse data with variances.");
  }
  void *end() {
    throw std::runtime_error(
        "`end` not implemented for sparse data with variances.");
  }

  constexpr auto size() const noexcept { return values.size(); }
};

template <class T> struct has_variances : std::false_type {};
template <class T>
struct has_variances<ValueAndVariance<T>> : std::true_type {};
template <class T>
struct has_variances<ValuesAndVariances<T>> : std::true_type {};
template <class T>
inline constexpr bool has_variances_v = has_variances<T>::value;

/// Helper for the transform implementation to unify iteration of data with and
/// without variances as well as sparse are dense container.
template <class T>
constexpr auto value_and_maybe_variance(const T &range, const scipp::index i) {
  if constexpr (has_variances_v<T>) {
    if constexpr (is_sparse_v<decltype(range.values[0])>)
      return ValuesAndVariances{range.values[i], range.variances[i]};
    else
      return ValueAndVariance{range.values[i], range.variances[i]};
  } else {
    return range[i];
  }
}

template <class T> struct is_eigen_type : std::false_type {};
template <class T, int Rows, int Cols>
struct is_eigen_type<Eigen::Matrix<T, Rows, Cols>> : std::true_type {};
template <class T, int Rows, int Cols>
struct is_eigen_type<sparse_container<Eigen::Matrix<T, Rows, Cols>>>
    : std::true_type {};
template <class T>
inline constexpr bool is_eigen_type_v = is_eigen_type<T>::value;

template <class Op, class T, class... Ts>
void transform_in_place_with_variance_impl(Op op, ValuesAndVariances<T> arg,
                                           Ts &&... other) {
  auto & [ vals, vars ] = arg;
  for (scipp::index i = 0; i < scipp::size(vals); ++i) {
    // Two cases are distinguished here:
    // 1. In the case of sparse data we create ValuesAndVariances, which hold
    //    references that can be modified.
    // 2. For dense data we create ValueAndVariance, which performs and element
    //    copy, so the result has to be updated after the call to `op`.
    // Note that in the case of sparse data we actually have a recursive call to
    // this function for the iteration over each individual sparse_container.
    // This then falls into case 2 and thus the recursion terminates with the
    // second level.
    if constexpr (is_sparse_v<decltype(vals[0])>) {
      op(ValuesAndVariances{vals[i], vars[i]},
         value_and_maybe_variance(other, i)...);
    } else {
      ValueAndVariance _{vals[i], vars[i]};
      op(_, value_and_maybe_variance(other, i)...);
      vals[i] = _.value;
      vars[i] = _.variance;
    }
  }
}

template <class Op, class Out, class... Ts>
void transform_elements_with_variance(Op op, ValuesAndVariances<Out> out,
                                      Ts &&... other) {
  auto & [ ovals, ovars ] = out;
  for (scipp::index i = 0; i < scipp::size(ovals); ++i) {
    if constexpr (is_sparse_v<decltype(ovals[0])>) {
      auto out_i = op(value_and_maybe_variance(other, i)...);
      ovals[i] = std::move(out_i.first);
      ovars[i] = std::move(out_i.second);
    } else {
      auto out_i = op(value_and_maybe_variance(other, i)...);
      ovals[i] = out_i.value;
      ovars[i] = out_i.variance;
    }
  }
}

template <class Op, class T, class... Ts>
void transform_in_place_impl(Op op, T &&vals, Ts &&... other) {
  for (scipp::index i = 0; i < scipp::size(vals); ++i)
    op(vals[i], other[i]...);
}

template <class Op, class Out, class T, class... Ts>
void transform_elements(Op op, Out &out, T &&vals, Ts &&... other) {
  for (scipp::index i = 0; i < scipp::size(out); ++i)
    out[i] = op(vals[i], other[i]...);
}

template <class T> struct element_type { using type = T; };
template <class T> struct element_type<sparse_container<T>> { using type = T; };
template <class T> struct element_type<const sparse_container<T>> {
  using type = T;
};
template <class T> struct element_type<ValueAndVariance<T>> { using type = T; };
template <class T>
struct element_type<ValuesAndVariances<sparse_container<T>>> {
  using type = T;
};
template <class T>
struct element_type<ValuesAndVariances<const sparse_container<T>>> {
  using type = T;
};
template <class T> using element_type_t = typename element_type<T>::type;

namespace transform_detail {
template <class T> struct is_sparse : std::false_type {};
template <class T> struct is_sparse<sparse_container<T>> : std::true_type {};
template <class T>
struct is_sparse<ValuesAndVariances<sparse_container<T>>> : std::true_type {};
template <class T>
struct is_sparse<ValuesAndVariances<const sparse_container<T>>>
    : std::true_type {};
template <class T> inline constexpr bool is_sparse_v = is_sparse<T>::value;
} // namespace transform_detail

/// Broadcast a constant to arbitrary size. Helper for TransformSparse.
///
/// This helper allows the use of a common transform implementation when mixing
/// sparse and non-sparse data.
template <class T> struct broadcast {
  constexpr auto operator[](const scipp::index) const noexcept { return value; }
  T value;
};
template <class T> broadcast(T)->broadcast<T>;

template <class T> decltype(auto) maybe_broadcast(T &&value) {
  if constexpr (transform_detail::is_sparse_v<
                    std::remove_const_t<std::remove_reference_t<T>>>)
    return std::forward<T>(value);
  else
    return broadcast{value};
}

template <class T> auto check_and_get_size(const T &a) {
  return scipp::size(a);
}

template <class T1, class T2>
auto check_and_get_size(const T1 &a, const T2 &b) {
  if constexpr (transform_detail::is_sparse_v<T1>) {
    if constexpr (transform_detail::is_sparse_v<T2>)
      expect::sizeMatches(a, b);
    return scipp::size(a);
  } else {
    return scipp::size(b);
  }
}

template <class T>
struct is_eigen_expression
    : std::is_base_of<Eigen::MatrixBase<std::decay_t<T>>, std::decay_t<T>> {};

template <class T> constexpr auto maybe_eval(T &&_) {
  if constexpr (is_eigen_expression<T>::value)
    return _.eval();
  else
    return std::forward<T>(_);
}

/// Functor for implementing in-place operations with sparse data.
///
/// This is (conditionally) added to an overloaded set of operators provided by
/// the user. If the data is sparse the overloads by this functor will match in
/// place of the user-provided ones. We then recursively call the transform
/// function. In this second call we have descended into the sparse container so
/// now the user-provided overload will match directly.
template <class Op> struct TransformSparseInPlace {
  Op op;
  template <class... Ts> constexpr void operator()(Ts &&... args) const {
    static_cast<void>(check_and_get_size(args...));
    if constexpr ((has_variances_v<Ts> || ...))
      transform_in_place_with_variance_impl(op, maybe_broadcast(args)...);
    else
      transform_in_place_impl(op, maybe_broadcast(args)...);
  }
};

/// Functor for implementing operations with sparse data, see also
/// TransformSparseInPlace.
template <class Op> struct TransformSparse {
  Op op;
  template <class... Ts> constexpr auto operator()(const Ts &... args) const {
    sparse_container<std::invoke_result_t<Op, element_type_t<Ts>...>> vals(
        check_and_get_size(args...));
    if constexpr ((has_variances_v<Ts> || ...)) {
      auto vars(vals);
      ValuesAndVariances out{vals, vars};
      transform_elements_with_variance(op, out, maybe_broadcast(args)...);
      return std::pair(std::move(vals), std::move(vars));
    } else {
      transform_elements(op, vals, maybe_broadcast(args)...);
      return vals;
    }
  }
};

/// Helper for in-place transform implementation, performing branching between
/// output with and without variances.
template <class T1, class Op> void do_transform_in_place(T1 &a, Op op) {
  auto a_val = a.values();
  if (a.hasVariances()) {
    if constexpr (is_eigen_type_v<typename T1::value_type>) {
      throw std::runtime_error("This dtype cannot have a variance.");
    } else {
      auto a_var = a.variances();
      transform_in_place_with_variance_impl(op,
                                            ValuesAndVariances{a_val, a_var});
    }
  } else {
    transform_in_place_impl(op, a_val);
  }
}

/// Helper for transform implementation, performing branching between output
/// with and without variances.
template <class T1, class Out, class Op>
void do_transform(const T1 &a, Out &out, Op op) {
  auto a_val = a.values();
  auto out_val = out.values();
  if (a.hasVariances()) {
    if constexpr (is_eigen_type_v<typename T1::value_type>) {
      throw std::runtime_error("This dtype cannot have a variance.");
    } else {
      auto a_var = a.variances();
      auto out_var = out.variances();
      transform_elements_with_variance(op, ValuesAndVariances{out_val, out_var},
                                       ValuesAndVariances{a_val, a_var});
    }
  } else {
    transform_elements(op, out_val, a_val);
  }
}

/// Helper for in-place transform implementation, performing branching between
/// output with and without variances as well as handling other operands with
/// and without variances.
template <class T1, class T2, class Op>
void do_transform_in_place(T1 &a, const T2 &b, Op op) {
  auto a_val = a.values();
  auto b_val = b.values();
  if (a.hasVariances()) {
    if constexpr (is_eigen_type_v<typename T1::value_type> ||
                  is_eigen_type_v<typename T2::value_type>) {
      throw std::runtime_error("This dtype cannot have a variance.");
    } else {
      auto a_var = a.variances();
      if (b.hasVariances()) {
        auto b_var = b.variances();
        transform_in_place_with_variance_impl(op,
                                              ValuesAndVariances{a_val, a_var},
                                              ValuesAndVariances{b_val, b_var});
      } else {
        transform_in_place_with_variance_impl(
            op, ValuesAndVariances{a_val, a_var}, b_val);
      }
    }
  } else if (b.hasVariances()) {
    throw std::runtime_error(
        "RHS in operation has variances but LHS does not.");
  } else {
    transform_in_place_impl(op, a_val, b_val);
  }
}

/// Helper for transform implementation, performing branching between output
/// with and without variances as well as handling other operands with and
/// without variances.
template <class T1, class T2, class Out, class Op>
void do_transform(const T1 &a, const T2 &b, Out &out, Op op) {
  auto a_val = a.values();
  auto b_val = b.values();
  auto out_val = out.values();
  if (a.hasVariances()) {
    if constexpr (is_eigen_type_v<typename T1::value_type> ||
                  is_eigen_type_v<typename T2::value_type>) {
      throw std::runtime_error("This dtype cannot have a variance.");
    } else {
      auto a_var = a.variances();
      auto out_var = out.variances();
      if (b.hasVariances()) {
        auto b_var = b.variances();
        transform_elements_with_variance(
            op, ValuesAndVariances{out_val, out_var},
            ValuesAndVariances{a_val, a_var}, ValuesAndVariances{b_val, b_var});
      } else {
        transform_elements_with_variance(
            op, ValuesAndVariances{out_val, out_var},
            ValuesAndVariances{a_val, a_var}, b_val);
      }
    }
  } else if (b.hasVariances()) {
    if constexpr (is_eigen_type_v<typename T2::value_type>) {
      throw std::runtime_error("This dtype cannot have a variance.");
    } else {
      auto b_var = b.variances();
      auto out_var = out.variances();
      transform_elements_with_variance(op, ValuesAndVariances{out_val, out_var},
                                       a_val, ValuesAndVariances{b_val, b_var});
    }
  } else {
    transform_elements(op, out_val, a_val, b_val);
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
template <class T> as_view(T &data, const Dimensions &dims)->as_view<T>;

/// Functor for in-place transformation, applying `op` to all elements.
///
/// This is responsible for converting the client-provided functor `Op` which
/// operates on elements to a functor for the data container, which is required
/// by `visit`.
template <class Op> struct TransformInPlace {
  Op op;
  template <class T> void operator()(T &&handle) const {
    auto view = as_view{*handle, handle->dims()};
    if (handle->isContiguous())
      do_transform_in_place(*handle, op);
    else
      do_transform_in_place(view, op);
  }

  template <class A, class B> void operator()(A &&a, B &&b_ptr) const {
    // std::unique_ptr::operator*() is const but returns mutable reference, need
    // to artificially put const to we call the correct overloads of ViewModel.
    // See #278.
    const auto &b = *b_ptr;
    const auto &dimsA = a->dims();
    const auto &dimsB = b.dims();
    if constexpr (std::is_same_v<decltype(*a), decltype(*b_ptr)>) {
      if (a->valuesView(dimsA).overlaps(b.valuesView(dimsA))) {
        // If there is an overlap between lhs and rhs we copy the rhs before
        // applying the operation.
        return operator()(a, b.copyT());
      }
    }

    if (a->isContiguous() && dimsA.contains(dimsB)) {
      if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
        do_transform_in_place(*a, b, op);
      } else {
        do_transform_in_place(*a, as_view{b, dimsA}, op);
      }
    } else if (dimsA.contains(dimsB)) {
      auto a_view = as_view{*a, dimsA};
      if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
        do_transform_in_place(a_view, b, op);
      } else {
        do_transform_in_place(a_view, as_view{b, dimsA}, op);
      }
    } else {
      // LHS has fewer dimensions than RHS, e.g., for computing sum. Use view.
      auto a_view = as_view{*a, dimsB};
      if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
        do_transform_in_place(a_view, b, op);
      } else {
        do_transform_in_place(a_view, as_view{b, dimsB}, op);
      }
    }
  }
};
template <class Op> TransformInPlace(Op)->TransformInPlace<Op>;

template <class Op> struct Transform {
  Op op;
  template <class T> Variable operator()(T &&handle) const {
    const auto &dims = handle->dims();
    using Out = decltype(maybe_eval(op(handle->values()[0])));
    // TODO For optimal performance we should just make container without
    // element init here.
    Variable out = handle->hasVariances()
                       ? makeVariableWithVariances<element_type_t<Out>>(dims)
                       : makeVariable<element_type_t<Out>>(dims);
    auto &outT = static_cast<VariableConceptT<Out> &>(out.data());
    do_transform(as_view{*handle, dims}, outT, op);
    return out;
  }

  template <class A, class B>
  Variable operator()(A &&a_handle, B &&b_handle) const {
    const auto &a = *a_handle;
    const auto &b = *b_handle;
    const auto &dimsA = a.dims();
    const auto &dimsB = b.dims();
    const auto &dims = merge(dimsA, dimsB);

    using Out = decltype(maybe_eval(op(a.values()[0], b.values()[0])));
    // TODO For optimal performance we should just make container without
    // element init here.
    Variable out = a.hasVariances() || b.hasVariances()
                       ? makeVariableWithVariances<element_type_t<Out>>(dims)
                       : makeVariable<element_type_t<Out>>(dims);
    auto &outT = static_cast<VariableConceptT<Out> &>(out.data());

    do_transform(as_view{a, dims}, as_view{b, dims}, outT, op);
    return out;
  }
};
template <class Op> Transform(Op)->Transform<Op>;

template <class T, class... Known> struct optional_sparse {
  using type = std::conditional_t<std::disjunction_v<std::is_same<T, Known>...>,
                                  std::tuple<T>, std::tuple<>>;
};

/// Augment a tuple of types with the corresponding sparse types, if they exist.
template <class... Ts, class... Known>
auto insert_sparse(const std::tuple<Ts...> &,
                   const VariableConceptHandle_impl<Known...> &) {
  return std::tuple_cat(
      std::tuple<Ts...>{},
      typename optional_sparse<sparse_container<Ts>, Known...>::type{}...);
}

template <class T1, class T2, class... Known> struct optional_sparse_pair {
  using type =
      std::conditional_t<std::disjunction_v<std::is_same<T1, Known>...> &&
                             std::disjunction_v<std::is_same<T2, Known>...>,
                         std::tuple<std::pair<T1, T2>>, std::tuple<>>;
};

/// Augment a tuple of type pairs with the corresponding sparse types, if they
/// exist.
template <class... Ts, class... Known>
auto insert_sparse_in_place_pairs(
    const std::tuple<Ts...> &, const VariableConceptHandle_impl<Known...> &) {
  return std::tuple_cat(
      std::tuple<Ts...>{},
      typename optional_sparse_pair<sparse_container<typename Ts::first_type>,
                                    typename Ts::second_type,
                                    Known...>::type{}...,
      typename optional_sparse_pair<sparse_container<typename Ts::first_type>,
                                    sparse_container<typename Ts::second_type>,
                                    Known...>::type{}...);
}
template <class... Ts, class... Known>
auto insert_sparse_pairs(const std::tuple<Ts...> &,
                         const VariableConceptHandle_impl<Known...> &) {
  return std::tuple_cat(
      std::tuple<Ts...>{},
      typename optional_sparse_pair<typename Ts::first_type,
                                    sparse_container<typename Ts::second_type>,
                                    Known...>::type{}...,
      typename optional_sparse_pair<sparse_container<typename Ts::first_type>,
                                    typename Ts::second_type,
                                    Known...>::type{}...,
      typename optional_sparse_pair<sparse_container<typename Ts::first_type>,
                                    sparse_container<typename Ts::second_type>,
                                    Known...>::type{}...);
}

template <class Op, class SparseOp> struct overloaded_sparse : Op, SparseOp {
  template <class... Ts> constexpr auto operator()(Ts &&... args) const {
    if constexpr ((transform_detail::is_sparse_v<
                       std::remove_const_t<std::remove_reference_t<Ts>>> ||
                   ...))
      return SparseOp::operator()(std::forward<Ts>(args)...);
    else
      return Op::operator()(std::forward<Ts>(args)...);
  }
};
template <class... Ts> overloaded_sparse(Ts...)->overloaded_sparse<Ts...>;

} // namespace detail

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;

/// Transform the data elements of a variable in-place.
///
/// Note that this is deliberately not named `for_each`: Unlike std::for_each,
/// this function does not promise in-order execution. This overload is
/// equivalent to std::transform with a single input range and an output range
/// identical to the input range, but avoids potentially costly element copies.
template <class... Ts, class Var, class Op>
void transform_in_place(Var &var, Op op) {
  using namespace detail;
  try {
    // If a sparse_container<T> is specified explicitly as a type we assume that
    // the caller provides a matching overload. Otherwise we assume the provided
    // operator is for individual elements (regardless of whether they are
    // elements of dense or sparse data), so we add overloads for sparse data
    // processing.
    if constexpr ((is_sparse_v<Ts> || ...)) {
      scipp::core::visit_impl<Ts...>::apply(TransformInPlace{op},
                                            var.dataHandle().variant());
    } else {
      scipp::core::visit(insert_sparse(std::tuple<Ts...>{}, var.dataHandle()))
          .apply(TransformInPlace{detail::overloaded_sparse{
                     op, TransformSparseInPlace<Op>{op}}},
                 var.dataHandle().variant());
    }
  } catch (const std::bad_variant_access &) {
    throw std::runtime_error("Operation not implemented for this type.");
  }
}

namespace detail {
template <class... Ts, class Var, class Var1, class Op>
void transform_in_place(std::tuple<Ts...> &&, Var &&var, const Var1 &other,
                        Op op) {
  using namespace detail;
  try {
    if constexpr (((is_sparse_v<typename Ts::first_type> ||
                    is_sparse_v<typename Ts::second_type>) ||
                   ...)) {
      scipp::core::visit_impl<Ts...>::apply(TransformInPlace{op},
                                            var.dataHandle().variant(),
                                            other.dataHandle().variant());
    } else {
      // Note that if only one of the inputs is sparse it must be the one being
      // transformed in-place, so there are only three cases here.
      scipp::core::visit(
          insert_sparse_in_place_pairs(std::tuple<Ts...>{}, var.dataHandle()))
          .apply(TransformInPlace{detail::overloaded_sparse{
                     op, TransformSparseInPlace<Op>{op}}},
                 var.dataHandle().variant(), other.dataHandle().variant());
    }
  } catch (const std::bad_variant_access &) {
    throw except::TypeError("Cannot apply operation to item dtypes " +
                            to_string(var.dtype()) + " and " +
                            to_string(other.dtype()) + '.');
  }
}
} // namespace detail

/// Transform the data elements of a variable in-place.
///
/// This overload is equivalent to std::transform with two input ranges and an
/// output range identical to the secound input range, but avoids potentially
/// costly element copies.
template <class... TypePairs, class Var, class Var1, class Op>
void transform_in_place(Var &&var, const Var1 &other, Op op) {
  // Wrapped implementation to convert multiple tuples into a parameter pack.
  detail::transform_in_place(std::tuple_cat(TypePairs{}...),
                             std::forward<Var>(var), other, op);
}

/// Transform the data elements of a variable and return a new Variable.
///
/// This overload is equivalent to std::transform with a single input range, but
/// avoids the need to manually create a new variable for the output and the
/// need for, e.g., std::back_inserter.
template <class... Ts, class Var, class Op>
Variable transform(const Var &var, Op op) {
  using namespace detail;
  try {
    if constexpr ((is_sparse_v<Ts> || ...)) {
      return scipp::core::visit_impl<Ts...>::apply(Transform{op},
                                                   var.dataHandle().variant());
    } else {
      return scipp::core::visit(
                 insert_sparse(std::tuple<Ts...>{}, var.dataHandle()))
          .apply(
              Transform{detail::overloaded_sparse{op, TransformSparse<Op>{op}}},
              var.dataHandle().variant());
    }
  } catch (const std::bad_variant_access &) {
    throw std::runtime_error("Operation not implemented for this type.");
  }
}

namespace detail {
template <class... Ts, class Var1, class Var2, class Op>
Variable transform(std::tuple<Ts...> &&, const Var1 &var1, const Var2 &var2,
                   Op op) {
  using namespace detail;
  try {
    if constexpr (((is_sparse_v<typename Ts::first_type> ||
                    is_sparse_v<typename Ts::second_type>) ||
                   ...)) {
      return scipp::core::visit_impl<Ts...>::apply(Transform{op},
                                                   var1.dataHandle().variant(),
                                                   var2.dataHandle().variant());
    } else {
      return scipp::core::visit(
                 insert_sparse_pairs(std::tuple<Ts...>{}, var1.dataHandle()))
          .apply(
              Transform{detail::overloaded_sparse{op, TransformSparse<Op>{op}}},
              var1.dataHandle().variant(), var2.dataHandle().variant());
    }
  } catch (const std::bad_variant_access &) {
    throw except::TypeError("Cannot apply operation to item dtypes " +
                            to_string(var1.dtype()) + " and " +
                            to_string(var2.dtype()) + '.');
  }
}
} // namespace detail

/// Transform the data elements of two variables and return a new Variable.
///
/// This overload is equivalent to std::transform with two input ranges, but
/// avoids the need to manually create a new variable for the output and the
/// need for, e.g., std::back_inserter.
template <class... TypePairs, class Var1, class Var2, class Op>
Variable transform(const Var1 &var1, const Var2 &var2, Op op) {
  // Wrapped implementation to convert multiple tuples into a parameter pack.
  return detail::transform(std::tuple_cat(TypePairs{}...), var1, var2, op);
}

} // namespace scipp::core

#endif // TRANSFORM_H
