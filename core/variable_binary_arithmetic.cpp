// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>
#include <iostream>

#include "scipp/core/dataset.h"
#include "scipp/core/except.h"
#include "scipp/core/tag_util.h"
#include "scipp/core/transform.h"
#include "scipp/core/variable.h"

#include "operators.h"

namespace scipp::core {

template <class T1, class T2> T1 &plus_equals(T1 &variable, const T2 &other) {
  // Note: This will broadcast/transpose the RHS if required. We do not support
  // changing the dimensions of the LHS though!
  transform_in_place(variable, other, operator_detail::plus_equals{});
  return variable;
}

template <class... Ts> struct pair_product {
  template <class T> struct pair_with {
    using type = decltype(std::tuple_cat(std::tuple<std::pair<T, Ts>>{}...));
  };
  using type = decltype(std::tuple_cat(typename pair_with<Ts>::type{}...));
};

template <class... Ts>
using pair_product_t = typename pair_product<Ts...>::type;

using arithmetic_type_pairs = pair_product_t<float, double, int32_t, int64_t>;

using arithmetic_and_matrix_type_pairs = decltype(std::tuple_cat(
    std::declval<arithmetic_type_pairs>(),
    std::tuple<std::pair<Eigen::Vector3d, Eigen::Vector3d>,
               std::pair<int64_t, int32_t>, std::pair<int32_t, int64_t>,
               std::pair<double, float>, std::pair<float, double>>()));

static constexpr auto plus_ = [](const auto a_, const auto b_) {
  return a_ + b_;
};
static constexpr auto minus_ = [](const auto a_, const auto b_) {
  return a_ - b_;
};
static constexpr auto times_ = [](const auto a_, const auto b_) {
  return a_ * b_;
};
static constexpr auto divide_ = [](const auto a_, const auto b_) {
  return a_ / b_;
};
template <class T1, class T2> Variable plus(const T1 &a, const T2 &b) {
  return transform<arithmetic_and_matrix_type_pairs>(a, b, plus_);
}

template <class T> class hasReciprocal {
  static void detect(...);
  template <class U> static decltype(reciprocal(std::declval<U>())) detect(U);

public:
  static constexpr bool value =
      !std::is_same_v<void, decltype(detect(std::declval<T>()))>;
};

static constexpr auto reciprocal_ = [](const auto a_) {
  if constexpr (hasReciprocal<std::decay_t<decltype(a_)>>::value)
    return reciprocal(a_);
  else
    return static_cast<decltype(a_)>(1) / a_;
};

Variable reciprocal(const VariableConstProxy &var) {
  return transform<double, float, int64_t, int32_t>(
      var, overloaded{reciprocal_, [](const units::Unit &unit) {
                        return units::Unit(units::dimensionless) / unit;
                      }});
}

Variable Variable::operator-() const {
  return transform<double, float, int64_t, Eigen::Vector3d>(
      *this, [](const auto a) { return -a; });
}

Variable &Variable::operator+=(const Variable &other) & {
  return plus_equals(*this, other);
}
Variable &Variable::operator+=(const VariableConstProxy &other) & {
  return plus_equals(*this, other);
}

template <class T1, class T2> T1 &minus_equals(T1 &variable, const T2 &other) {
  transform_in_place(variable, other, operator_detail::minus_equals{});
  return variable;
}

template <class T1, class T2> Variable minus(const T1 &a, const T2 &b) {
  return transform<arithmetic_and_matrix_type_pairs>(a, b, minus_);
}

Variable &Variable::operator-=(const Variable &other) & {
  return minus_equals(*this, other);
}
Variable &Variable::operator-=(const VariableConstProxy &other) & {
  return minus_equals(*this, other);
}

template <class T1, class T2> T1 &times_equals(T1 &variable, const T2 &other) {
  transform_in_place(variable, other, operator_detail::times_equals{});
  return variable;
}

template <class T1, class T2> Variable times(const T1 &a, const T2 &b) {
  return transform<arithmetic_type_pairs>(a, b, times_);
}

Variable &Variable::operator*=(const Variable &other) & {
  return times_equals(*this, other);
}
Variable &Variable::operator*=(const VariableConstProxy &other) & {
  return times_equals(*this, other);
}

template <class T1, class T2> T1 &divide_equals(T1 &variable, const T2 &other) {
  transform_in_place(variable, other, operator_detail::divide_equals{});
  return variable;
}

template <class T1, class T2> Variable divide(const T1 &a, const T2 &b) {
  return transform<arithmetic_type_pairs>(a, b, divide_);
}

Variable &Variable::operator/=(const Variable &other) & {
  return divide_equals(*this, other);
}
Variable &Variable::operator/=(const VariableConstProxy &other) & {
  return divide_equals(*this, other);
}

template <class T1, class T2> T1 &or_equals(T1 &variable, const T2 &other) {
  transform_in_place<pair_self_t<bool>>(
      variable, other,
      overloaded{[](auto &var_, const auto &other_) { var_ |= other_; },
                 [](units::Unit &varUnit, const units::Unit &otherUnit) {
                   expect::equals(varUnit, units::dimensionless);
                   expect::equals(otherUnit, units::dimensionless);
                 }});

  return variable;
}

template <class T1, class T2> Variable or_op(const T1 &a, const T2 &b) {
  return transform<pair_self_t<bool>>(
      a, b,
      overloaded{[](const auto &var_, const auto &other_) -> bool {
                   return var_ | other_;
                 },
                 [](const units::Unit &aUnit, const units::Unit &bUnit) {
                   expect::equals(aUnit, units::dimensionless);
                   expect::equals(bUnit, units::dimensionless);
                   return aUnit;
                 }});
}

Variable &Variable::operator|=(const Variable &other) & {
  return or_equals(*this, other);
}
Variable &Variable::operator|=(const VariableConstProxy &other) & {
  return or_equals(*this, other);
}

VariableProxy VariableProxy::operator+=(const Variable &other) const {
  return plus_equals(*this, other);
}
VariableProxy VariableProxy::operator+=(const VariableConstProxy &other) const {
  return plus_equals(*this, other);
}

VariableProxy VariableProxy::operator-=(const Variable &other) const {
  return minus_equals(*this, other);
}
VariableProxy VariableProxy::operator-=(const VariableConstProxy &other) const {
  return minus_equals(*this, other);
}

VariableProxy VariableProxy::operator*=(const Variable &other) const {
  return times_equals(*this, other);
}
VariableProxy VariableProxy::operator*=(const VariableConstProxy &other) const {
  return times_equals(*this, other);
}

VariableProxy VariableProxy::operator/=(const Variable &other) const {
  return divide_equals(*this, other);
}
VariableProxy VariableProxy::operator/=(const VariableConstProxy &other) const {
  return divide_equals(*this, other);
}

VariableProxy VariableProxy::operator|=(const Variable &other) const {
  return or_equals(*this, other);
}
VariableProxy VariableProxy::operator|=(const VariableConstProxy &other) const {
  return or_equals(*this, other);
}

Variable VariableConstProxy::operator-() const {
  Variable copy(*this);
  return -copy;
}

Variable operator+(const Variable &a, const Variable &b) { return plus(a, b); }
Variable operator-(const Variable &a, const Variable &b) { return minus(a, b); }
Variable operator*(const Variable &a, const Variable &b) { return times(a, b); }
Variable operator/(const Variable &a, const Variable &b) {
  return divide(a, b);
}
Variable operator|(const Variable &a, const Variable &b) { return or_op(a, b); }
Variable operator+(const Variable &a, const VariableConstProxy &b) {
  return plus(a, b);
}
Variable operator-(const Variable &a, const VariableConstProxy &b) {
  return minus(a, b);
}
Variable operator*(const Variable &a, const VariableConstProxy &b) {
  return times(a, b);
}
Variable operator/(const Variable &a, const VariableConstProxy &b) {
  return divide(a, b);
}
Variable operator|(const Variable &a, const VariableConstProxy &b) {
  return or_op(a, b);
}
Variable operator+(const VariableConstProxy &a, const Variable &b) {
  return plus(a, b);
}
Variable operator-(const VariableConstProxy &a, const Variable &b) {
  return minus(a, b);
}
Variable operator*(const VariableConstProxy &a, const Variable &b) {
  return times(a, b);
}
Variable operator/(const VariableConstProxy &a, const Variable &b) {
  return divide(a, b);
}
Variable operator|(const VariableConstProxy &a, const Variable &b) {
  return or_op(a, b);
}
Variable operator+(const VariableConstProxy &a, const VariableConstProxy &b) {
  return plus(a, b);
}
Variable operator-(const VariableConstProxy &a, const VariableConstProxy &b) {
  return minus(a, b);
}
Variable operator*(const VariableConstProxy &a, const VariableConstProxy &b) {
  return times(a, b);
}
Variable operator/(const VariableConstProxy &a, const VariableConstProxy &b) {
  return divide(a, b);
}
Variable operator|(const VariableConstProxy &a, const VariableConstProxy &b) {
  return or_op(a, b);
}
Variable operator+(const VariableConstProxy &a_, const double b) {
  Variable a(a_);
  return a += b;
}
Variable operator-(const VariableConstProxy &a_, const double b) {
  Variable a(a_);
  return a -= b;
}
Variable operator*(const VariableConstProxy &a_, const double b) {
  Variable a(a_);
  return a *= b;
}
Variable operator/(const VariableConstProxy &a_, const double b) {
  Variable a(a_);
  return a /= b;
}
Variable operator+(const double a, const VariableConstProxy &b_) {
  Variable b(b_);
  return b += a;
}
Variable operator-(const double a, const VariableConstProxy &b_) {
  Variable b(b_);
  return -(b -= a);
}
Variable operator*(const double a, const VariableConstProxy &b_) {
  Variable b(b_);
  return b *= a;
}
Variable operator/(const double a, const VariableConstProxy &b_proxy) {
  Variable b(b_proxy);
  transform_in_place<double, float>(
      b,
      overloaded{
          [](units::Unit &b_) { b_ = units::Unit(units::dimensionless) / b_; },
          [a](double &b_) { b_ = a / b_; },
          [a](float &b_) { b_ = static_cast<float>(a / b_); },
          [a](auto &b_) {
            b_ = static_cast<std::remove_reference_t<decltype(b_)>>(a / b_);
          }});
  return b;
}

struct MakeVariableWithType {
  template <class T> struct Maker {
    static Variable apply(const VariableConstProxy &parent) {
     return transform<double, float, int64_t, int32_t, bool>(parent, overloaded{
       [](const units::Unit &x){ return x; },
       [](const auto &x){
         if constexpr (detail::is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
          return detail::ValueAndVariance<T>{static_cast<T>(x.value), static_cast<T>(x.variance)};
         else
           return static_cast<T>(x);
       }});
    }
  };
  static Variable make(const VariableConstProxy &var, DType type) {
    return CallDType<double, float, int64_t, int32_t, bool>::apply<Maker>(type, var);
  }
};

Variable astype(const VariableConstProxy &var, DType type) {
  return MakeVariableWithType::make(var, type);
}

} // namespace scipp::core
