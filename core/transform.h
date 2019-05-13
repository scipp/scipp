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
};

template <class T1, class T2>
constexpr auto operator+(const ValueAndVariance<T1> &a,
                         const ValueAndVariance<T2> &b) noexcept {
  return ValueAndVariance{a.value + b.value, a.variance + b.variance};
}
template <class T1, class T2>
constexpr auto operator-(const ValueAndVariance<T1> &a,
                         const ValueAndVariance<T2> &b) noexcept {
  return ValueAndVariance{a.value - b.value, a.variance - b.variance};
}
template <class T1, class T2>
constexpr auto operator*(const ValueAndVariance<T1> &a,
                         const ValueAndVariance<T2> &b) noexcept {
  return ValueAndVariance{a.value * b.value,
                          a.variance * b.value * b.value +
                              b.variance * a.value * a.value};
}
template <class T1, class T2>
constexpr auto operator/(const ValueAndVariance<T1> &a,
                         const ValueAndVariance<T2> &b) noexcept {
  return ValueAndVariance{a.value / b.value,
                          a.variance / (b.value * b.value) +
                              b.variance / (a.value * a.value)};
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
  template <class T> constexpr auto operator()(sparse_container<T> x) const {
    std::transform(x.begin(), x.end(), x.begin(), op);
    return x;
  }
  // TODO Would like to use T1 and T2 for a and b, but currently this leads to
  // selection of the wrong overloads.
  template <class T>
  constexpr auto operator()(sparse_container<T> a, const T b) const {
    std::transform(a.begin(), a.end(), a.begin(),
                   [&, b](const T a) { return op(a, b); });
    return a;
  }
};

template <class T1, class T2, class Op>
void transform_with_variance(const T1 &a, const T2 &b, T1 &c, Op op) {
  auto a_val = a.values();
  auto a_var = a.variances();
  auto b_val = b.values();
  auto b_var = b.variances();
  auto c_val = c.values();
  auto c_var = c.variances();
  for (scipp::index i = 0; i < a_val.size(); ++i) {
    const ValueAndVariance a_{a_val[i], a_var[i]};
    const ValueAndVariance b_{b_val[i], b_var[i]};
    const auto out = op(a_, b_);
    c_val[i] = out.value;
    c_var[i] = out.variance;
  }
}

template <class T1, class T2, class Op>
void transform_with_variance(const VariableConceptT<sparse_container<T1>> &a,
                             const T2 &b,
                             VariableConceptT<sparse_container<T1>> &c, Op op) {
}

template <class Op> struct TransformInPlace {
  Op op;
  template <class T> void operator()(T &&handle) const {
    auto data = handle->values();
    std::transform(data.begin(), data.end(), data.begin(), op);
  }
  template <class A, class B> void operator()(A &&a, B &&b_ptr) const {
    // std::unique_ptr::operator*() is const but returns mutable reference, need
    // to artificially put const to we call the correct overloads of ViewModel.
    const auto &b = *b_ptr;
    const auto &dimsA = a->dimensions();
    const auto &dimsB = b.dimensions();
    try {
      if constexpr (std::is_same_v<decltype(*a), decltype(*b_ptr)>) {
        if (a->valuesView(dimsA).overlaps(b.valuesView(dimsA))) {
          // If there is an overlap between lhs and rhs we copy the rhs before
          // applying the operation.
          const auto &data = b.valuesView(b.dimensions());
          using T = typename std::remove_reference_t<decltype(b)>::value_type;
          const std::unique_ptr<VariableConceptT<T>> copy =
              detail::makeVariableConceptT<T>(
                  dimsB, Vector<T>(data.begin(), data.end()));
          return operator()(a, copy);
        }
      }

      if (a->isContiguous() && dimsA.contains(dimsB)) {
        if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
          if (a->hasVariances()) {
            if constexpr (std::is_same_v<typename std::remove_reference_t<
                                             decltype(*a)>::value_type,
                                         double> &&
                          std::is_same_v<typename std::remove_reference_t<
                                             decltype(b)>::value_type,
                                         double>)
              transform_with_variance(*a, b, *a, op);
            else
              throw std::runtime_error("This dtype cannot have a variance.");
          } else {
            auto a_ = a->values();
            auto b_ = b.values();
            std::transform(a_.begin(), a_.end(), b_.begin(), a_.begin(), op);
          }
        } else {
          auto a_ = a->values();
          auto b_ = b.valuesView(dimsA);
          std::transform(a_.begin(), a_.end(), b_.begin(), a_.begin(), op);
        }
      } else if (dimsA.contains(dimsB)) {
        if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
          auto a_ = a->valuesView(dimsA);
          auto b_ = b.values();
          std::transform(a_.begin(), a_.end(), b_.begin(), a_.begin(), op);
        } else {
          auto a_ = a->valuesView(dimsA);
          auto b_ = b.valuesView(dimsA);
          std::transform(a_.begin(), a_.end(), b_.begin(), a_.begin(), op);
        }
      } else {
        // LHS has fewer dimensions than RHS, e.g., for computing sum. Use view.
        if (b.isContiguous() && dimsA.isContiguousIn(dimsB)) {
          auto a_ = a->valuesView(dimsB);
          auto b_ = b.values();
          std::transform(a_.begin(), a_.end(), b_.begin(), a_.begin(), op);
        } else {
          auto a_ = a->valuesView(dimsB);
          auto b_ = b.valuesView(dimsB);
          std::transform(a_.begin(), a_.end(), b_.begin(), a_.begin(), op);
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
    auto data = handle->values();
    // TODO Should just make empty container here, without init.
    auto out = detail::makeVariableConceptT<decltype(op(*data.begin()))>(
        handle->dimensions());
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
//
// Note that this is deliberately not named `for_each`: Unlike std::for_each,
// this function does not promise in-order execution. This overload is
// equivalent to std::transform with a single input range and an output range
// identical to the input range, but avoids potentially costly element copies.
template <class... Ts, class Var, class Op>
void transform_in_place(Var &var, Op op) {
  using namespace detail;
  try {
    scipp::core::visit_impl<Ts...>::apply(
        TransformInPlace{overloaded{op, TransformSparse<Op>{op}}},
        var.dataHandle().variant());
  } catch (const std::bad_variant_access &) {
    throw std::runtime_error("Operation not implemented for this type.");
  }
}

/// Transform the data elements of a variable in-place.
//
// This overload is equivalent to std::transform with two input ranges and an
// output range identical to the secound input range, but avoids potentially
// costly element copies.
template <class... TypePairs, class Var1, class Var, class Op>
void transform_in_place(const Var1 &other, Var &&var, Op op) {
  using namespace detail;
  try {
    scipp::core::visit(std::tuple_cat(TypePairs{}...))
        .apply(TransformInPlace{overloaded{op, TransformSparse<Op>{op}}},
               var.dataHandle().variant(), other.dataHandle().variant());
  } catch (const std::bad_variant_access &) {
    throw except::TypeError("Cannot apply operation to item dtypes " +
                            to_string(var.dtype()) + " and " +
                            to_string(other.dtype()) + '.');
  }
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
