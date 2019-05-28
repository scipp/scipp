// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "except.h"
#include "variable.h"
#include "visit.h"

namespace scipp::core {

namespace detail {

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
constexpr auto operator-(const ValueAndVariance<T1> a, const T2 b) noexcept {
  return ValueAndVariance{a.value - b, a.variance};
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

template <class T>
ValueAndVariance(const T &val, const T &var)->ValueAndVariance<T>;

template <class T>
std::unique_ptr<VariableConceptT<T>>
makeVariableConceptT(const Dimensions &dims);
template <class T>
std::unique_ptr<VariableConceptT<T>>
makeVariableConceptT(const Dimensions &dims, Vector<T> data);

template <class Op> struct TransformSparse {
  Op op;
  // TODO avoid copies... need in place transform (for_each, but with a second
  // input range).
  template <class T> constexpr void operator()(sparse_container<T> &x) const {
    for (auto &x_ : x)
      op(x_);
  }
  template <class T1, class T2>
  constexpr void operator()(sparse_container<T1> &a, const T2 b) const {
    for (scipp::index i = 0; i < scipp::size(a); ++i)
      op(a[i], b);
  }
  template <class T1, class T2>
  constexpr void operator()(sparse_container<T1> &a,
                            const sparse_container<T2> &b) const {
    if (scipp::size(a) != scipp::size(b))
      throw std::runtime_error("Mismatch in extent of sparse dimension.");
    for (scipp::index i = 0; i < scipp::size(a); ++i)
      op(a[i], b[i]);
  }
};

template <class T> struct is_eigen_type : std::false_type {};
template <class T, int Rows, int Cols>
struct is_eigen_type<Eigen::Matrix<T, Rows, Cols>> : std::true_type {};
template <class T>
inline constexpr bool is_eigen_type_v = is_eigen_type<T>::value;

template <class T> struct as_view {
  using value_type = typename T::value_type;
  bool hasVariances() const { return data.hasVariances(); }
  auto values() const { return data.valuesView(dims); }
  auto variances() const { return data.variancesView(dims); }

  T &data;
  const Dimensions &dims;
};
template <class T> as_view(T &data, const Dimensions &dims)->as_view<T>;

template <class T1, class Op> void do_transform(T1 &a, Op op) {
  auto a_val = a.values();
  if (a.hasVariances()) {
    if constexpr (is_sparse_v<typename T1::value_type>) {
      throw std::runtime_error(
          "Propagation of uncertainties for sparse data not implemented yet.");
    } else if constexpr (is_eigen_type_v<typename T1::value_type>) {
      throw std::runtime_error("This dtype cannot have a variance.");
    } else {
      auto a_var = a.variances();
      for (scipp::index i = 0; i < a_val.size(); ++i) {
        ValueAndVariance a_{a_val[i], a_var[i]};
        op(a_);
        a_val[i] = a_.value;
        a_var[i] = a_.variance;
      }
    }
  } else {
    for (auto &a_ : a_val)
      op(a_);
  }
}

template <class T1, class T2, class Op>
void do_transform(T1 &a, const T2 &b, Op op) {
  auto a_val = a.values();
  auto b_val = b.values();
  if (a.hasVariances()) {
    if constexpr (is_sparse_v<typename T1::value_type> ||
                  is_sparse_v<typename T2::value_type>) {
      throw std::runtime_error(
          "Propagation of uncertainties for sparse data not implemented yet.");
    } else if constexpr (is_eigen_type_v<typename T1::value_type> ||
                         is_eigen_type_v<typename T2::value_type>) {
      throw std::runtime_error("This dtype cannot have a variance.");
    } else {
      auto a_var = a.variances();
      if (b.hasVariances()) {
        auto b_var = b.variances();
        for (scipp::index i = 0; i < a_val.size(); ++i) {
          ValueAndVariance a_{a_val[i], a_var[i]};
          const ValueAndVariance b_{b_val[i], b_var[i]};
          op(a_, b_);
          a_val[i] = a_.value;
          a_var[i] = a_.variance;
        }
      } else {
        for (scipp::index i = 0; i < a_val.size(); ++i) {
          ValueAndVariance a_{a_val[i], a_var[i]};
          op(a_, b_val[i]);
          a_val[i] = a_.value;
          a_var[i] = a_.variance;
        }
      }
    }
  } else if (b.hasVariances()) {
    throw std::runtime_error(
        "RHS in operation has variances but LHS does not.");
  } else {
    for (scipp::index i = 0; i < a_val.size(); ++i)
      op(a_val[i], b_val[i]);
  }
}

template <class Op> struct TransformInPlace {
  Op op;
  /// Transform container with concrete type T with operation `op`.
  ///
  /// This is called by `visit` from the top-level `transform` or
  /// `transform_in_place` functions.
  template <class T> void operator()(T &&handle) const {
    auto view = as_view{*handle, handle->dims()};
    if (handle->isContiguous())
      do_transform(*handle, op);
    else
      do_transform(view, op);
  }

  template <class A, class B> void operator()(A &&a, B &&b_ptr) const {
    // std::unique_ptr::operator*() is const but returns mutable reference, need
    // to artificially put const to we call the correct overloads of ViewModel.
    const auto &b = *b_ptr;
    const auto &dimsA = a->dims();
    const auto &dimsB = b.dims();
    try {
      if constexpr (std::is_same_v<decltype(*a), decltype(*b_ptr)>) {
        if (a->valuesView(dimsA).overlaps(b.valuesView(dimsA))) {
          // If there is an overlap between lhs and rhs we copy the rhs before
          // applying the operation.
          const auto &data = b.valuesView(b.dims());
          using T = typename std::remove_reference_t<decltype(b)>::value_type;
          const std::unique_ptr<VariableConceptT<T>> copy =
              detail::makeVariableConceptT<T>(
                  dimsB, Vector<T>(data.begin(), data.end()));
          return operator()(a, copy);
        }
      }

      if (a->isContiguous() && dimsA.contains(dimsB)) {
        if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
          do_transform(*a, b, op);
        } else {
          do_transform(*a, as_view{b, dimsA}, op);
        }
      } else if (dimsA.contains(dimsB)) {
        auto a_view = as_view{*a, dimsA};
        if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
          do_transform(a_view, b, op);
        } else {
          do_transform(a_view, as_view{b, dimsA}, op);
        }
      } else {
        // LHS has fewer dimensions than RHS, e.g., for computing sum. Use view.
        auto a_view = as_view{*a, dimsB};
        if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
          do_transform(a_view, b, op);
        } else {
          do_transform(a_view, as_view{b, dimsB}, op);
        }
      }
    } catch (const std::bad_cast &) {
      throw std::runtime_error("Cannot apply arithmetic operation to "
                               "Variables: Underlying data types do not "
                               "match.");
    }
  }
};
template <class Op> TransformInPlace(Op)->TransformInPlace<Op>;

template <class Op> struct Transform {
  Op op;
  template <class T> VariableConceptHandle operator()(T &&handle) const {
    if (handle->hasVariances())
      throw std::runtime_error(
          "Propgation of uncertainties not implemented for this case.");
    auto data = handle->values();
    // TODO Should just make empty container here, without init.
    auto out = detail::makeVariableConceptT<decltype(op(*data.begin()))>(
        handle->dims());
    // TODO Typo data->values() also compiles, but const-correctness should
    // forbid this.
    auto outData = out->values();
    std::transform(data.begin(), data.end(), outData.begin(), op);
    return {std::move(out)};
  }
};

} // namespace detail

template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;

/// Transform the data elements of a variable in-place.
///
/// Note that this is deliberately not named `for_each`: Unlike std::for_each,
/// this function does not promise in-order execution. This overload is
/// equivalent to std::transform with a single input range and an output range
/// identical to the input range, but avoids potentially costly element copies.
///
/// The underlying mechanism is as follows:
/// 1. `visit` (or `visit_impl`) obtains the concrete underlying data type.
/// 2. `TransformInPlace` is applied to that concrete container, calling
///    `do_transform`. `TransformInPlace` essentially builds a callable
///    accepting a container from a callable accepting and element of the
///    container.
/// 3. `do_transform` is essentially a fancy std::transform. It provides
///    automatic handling of data that has variances in addition to values.
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
      scipp::core::visit_impl<Ts..., sparse_container<Ts>...>::apply(
          TransformInPlace{overloaded{op, TransformSparse<Op>{op}}},
          var.dataHandle().variant());
    }
  } catch (const std::bad_variant_access &) {
    throw std::runtime_error("Operation not implemented for this type.");
  }
}

template <class... Ts, class Var, class Var1, class Op>
void do_transform_in_place(std::tuple<Ts...> &&, Var &&var, const Var1 &other,
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
      scipp::core::visit_impl<
          Ts...,
          std::pair<sparse_container<typename Ts::first_type>,
                    typename Ts::second_type>...,
          std::pair<sparse_container<typename Ts::first_type>,
                    sparse_container<typename Ts::second_type>>...>::
          apply(TransformInPlace{overloaded{op, TransformSparse<Op>{op}}},
                var.dataHandle().variant(), other.dataHandle().variant());
    }
  } catch (const std::bad_variant_access &) {
    throw except::TypeError("Cannot apply operation to item dtypes " +
                            to_string(var.dtype()) + " and " +
                            to_string(other.dtype()) + '.');
  }
}

/// Transform the data elements of a variable in-place.
//
// This overload is equivalent to std::transform with two input ranges and an
// output range identical to the secound input range, but avoids potentially
// costly element copies.
template <class... TypePairs, class Var, class Var1, class Op>
void transform_in_place(Var &&var, const Var1 &other, Op op) {
  do_transform_in_place(std::tuple_cat(TypePairs{}...), std::forward<Var>(var),
                        other, op);
}

/// Transform the data elements of a variable and return a new Variable.
//
// This overload is equivalent to std::transform with a single input range, but
// avoids the need to manually create a new variable for the output and the need
// for, e.g., std::back_inserter.
template <class... Ts, class Var, class Op>
Variable transform(const Var &var, Op op) {
  using namespace detail;
  try {
    return Variable(var, scipp::core::visit_impl<Ts...>::apply(
                             Transform<Op>{op}, var.dataHandle().variant()));
  } catch (const std::bad_variant_access &) {
    throw std::runtime_error("Operation not implemented for this type.");
  }
}

} // namespace scipp::core

#endif // TRANSFORM_H
