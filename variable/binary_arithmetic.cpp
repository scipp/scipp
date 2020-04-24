// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <cmath>

#include "scipp/variable/binary_arithmetic.h"
#include "scipp/variable/transform.h"

using namespace scipp::core;

namespace scipp::variable {

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

Variable Variable::operator-() const {
  return transform<double, float, int64_t, int32_t, Eigen::Vector3d>(
      *this, [](const auto a) { return -a; });
}

template <class T1, class T2> Variable minus(const T1 &a, const T2 &b) {
  return transform<arithmetic_and_matrix_type_pairs>(a, b, minus_);
}

template <class T1, class T2> Variable times(const T1 &a, const T2 &b) {
  return transform<arithmetic_type_pairs_with_bool>(a, b, times_);
}

template <class T1, class T2> Variable divide(const T1 &a, const T2 &b) {
  return transform<arithmetic_type_pairs>(a, b, divide_);
}

Variable VariableConstView::operator-() const {
  Variable copy(*this);
  return -copy;
}

Variable operator+(const VariableConstView &a, const VariableConstView &b) {
  return plus(a, b);
}
Variable operator-(const VariableConstView &a, const VariableConstView &b) {
  return minus(a, b);
}
Variable operator*(const VariableConstView &a, const VariableConstView &b) {
  return times(a, b);
}
Variable operator/(const VariableConstView &a, const VariableConstView &b) {
  return divide(a, b);
}

} // namespace scipp::variable
