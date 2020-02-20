// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/core/transform.h"

#include "operators.h"

namespace scipp::core {

template <class Op>
void dry_run_op(const DataArrayView &a, const VariableConstView &b, Op op) {
  // This dry run relies on the knowledge that the implementation of operations
  // for variable simply calls transform_in_place and nothing else.
  dry_run::transform_in_place(a.data(), b, op);
}

template <class Op>
void dry_run_op(const DataArrayView &a, const DataArrayConstView &b, Op op) {
  expect::coordsAndLabelsAreSuperset(a, b);
  dry_run_op(a, b.data(), op);
}

DataArrayView DataArrayView::operator+=(const DataArrayConstView &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  data() += other.data();
  return *this;
}

DataArrayView DataArrayView::operator-=(const DataArrayConstView &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  data() -= other.data();
  return *this;
}

DataArrayView DataArrayView::operator*=(const DataArrayConstView &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  data() *= other.data();
  return *this;
}

DataArrayView DataArrayView::operator/=(const DataArrayConstView &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  data() /= other.data();
  return *this;
}

DataArrayView DataArrayView::operator+=(const VariableConstView &other) const {
  data() += other;
  return *this;
}

DataArrayView DataArrayView::operator-=(const VariableConstView &other) const {
  data() -= other;
  return *this;
}

DataArrayView DataArrayView::operator*=(const VariableConstView &other) const {
  data() *= other;
  return *this;
}

DataArrayView DataArrayView::operator/=(const VariableConstView &other) const {
  data() /= other;
  return *this;
}

constexpr static auto plus = [](const auto &a, const auto &b) { return a + b; };

constexpr static auto minus = [](const auto &a, const auto &b) {
  return a - b;
};

constexpr static auto times = [](const auto &a, const auto &b) {
  return a * b;
};

constexpr static auto divide = [](const auto &a, const auto &b) {
  return a / b;
};

template <class Op, class A, class B>
auto &apply(const Op &op, A &a, const B &b) {
  for (const auto &item : b)
    dry_run_op(a[item.name()], item, op);
  for (const auto &item : b)
    op(a[item.name()], item);
  return a;
}

template <typename T> bool are_same(const T &a, const T &b) { return &a == &b; }

template <class A, class B>
bool have_common_underlying(const A &a, const B &b) {
  return are_same(a.underlying(), b.underlying());
}

template <>
bool have_common_underlying<DataArrayView, VariableConstView>(
    const DataArrayView &a, const VariableConstView &b) {
  return are_same(*a.underlying().data, b.underlying());
}

template <class Op, class A, class B>
decltype(auto) apply_with_delay(const Op &op, A &&a, const B &b) {
  for (const auto &item : a)
    dry_run_op(item, b, op);
  // For `b` referencing data in `a` we delay operation. The alternative would
  // be to make a deep copy of `other` before starting the iteration over items.
  std::optional<DataArrayView> delayed;
  // Note the inefficiency here: We are comparing some or all of the coords and
  // labels for each item. This could be improved by implementing the operations
  // for detail::DatasetData instead of DataArrayView.
  for (const auto &item : a) {
    if (have_common_underlying(item, b))
      delayed = item;
    else
      op(item, b);
  }
  if (delayed)
    op(*delayed, b);
  return std::forward<A>(a);
}

template <class Op, class A, class B>
auto apply_with_broadcast(const Op &op, const A &a, const B &b) {
  Dataset res;
  for (const auto &item : b)
    if (const auto it = a.find(item.name()); it != a.end())
      res.setData(item.name(), op(*it, item));
  return res;
}

template <class Op, class A>
auto apply_with_broadcast(const Op &op, const A &a,
                          const DataArrayConstView &b) {
  Dataset res;
  for (const auto &item : a)
    res.setData(item.name(), op(item, b));
  return res;
}

template <class Op, class B>
auto apply_with_broadcast(const Op &op, const DataArrayConstView &a,
                          const B &b) {
  Dataset res;
  for (const auto &item : b)
    res.setData(item.name(), op(a, item));
  return res;
}

template <class Op, class A>
auto apply_with_broadcast(const Op &op, const A &a,
                          const VariableConstView &b) {
  Dataset res;
  for (const auto &item : a)
    res.setData(item.name(), op(item, b));
  return res;
}

template <class Op, class B>
auto apply_with_broadcast(const Op &op, const VariableConstView &a,
                          const B &b) {
  Dataset res;
  for (const auto &item : b)
    res.setData(item.name(), op(a, item));
  return res;
}

Dataset &Dataset::operator+=(const DataArrayConstView &other) {
  return apply_with_delay(operator_detail::plus_equals{}, *this, other);
}

Dataset &Dataset::operator-=(const DataArrayConstView &other) {
  return apply_with_delay(operator_detail::minus_equals{}, *this, other);
}

Dataset &Dataset::operator*=(const DataArrayConstView &other) {
  return apply_with_delay(operator_detail::times_equals{}, *this, other);
}

Dataset &Dataset::operator/=(const DataArrayConstView &other) {
  return apply_with_delay(operator_detail::divide_equals{}, *this, other);
}

Dataset &Dataset::operator+=(const VariableConstView &other) {
  return apply_with_delay(operator_detail::plus_equals{}, *this, other);
}

Dataset &Dataset::operator-=(const VariableConstView &other) {
  return apply_with_delay(operator_detail::minus_equals{}, *this, other);
}

Dataset &Dataset::operator*=(const VariableConstView &other) {
  return apply_with_delay(operator_detail::times_equals{}, *this, other);
}

Dataset &Dataset::operator/=(const VariableConstView &other) {
  return apply_with_delay(operator_detail::divide_equals{}, *this, other);
}

Dataset &Dataset::operator+=(const DatasetConstView &other) {
  return apply(operator_detail::plus_equals{}, *this, other);
}

Dataset &Dataset::operator-=(const DatasetConstView &other) {
  return apply(operator_detail::minus_equals{}, *this, other);
}

Dataset &Dataset::operator*=(const DatasetConstView &other) {
  return apply(operator_detail::times_equals{}, *this, other);
}

Dataset &Dataset::operator/=(const DatasetConstView &other) {
  return apply(operator_detail::divide_equals{}, *this, other);
}

Dataset &Dataset::operator+=(const Dataset &other) {
  return apply(operator_detail::plus_equals{}, *this, other);
}

Dataset &Dataset::operator-=(const Dataset &other) {
  return apply(operator_detail::minus_equals{}, *this, other);
}

Dataset &Dataset::operator*=(const Dataset &other) {
  return apply(operator_detail::times_equals{}, *this, other);
}

Dataset &Dataset::operator/=(const Dataset &other) {
  return apply(operator_detail::divide_equals{}, *this, other);
}

DatasetView DatasetView::operator+=(const DataArrayConstView &other) const {
  return apply_with_delay(operator_detail::plus_equals{}, *this, other);
}

DatasetView DatasetView::operator-=(const DataArrayConstView &other) const {
  return apply_with_delay(operator_detail::minus_equals{}, *this, other);
}

DatasetView DatasetView::operator*=(const DataArrayConstView &other) const {
  return apply_with_delay(operator_detail::times_equals{}, *this, other);
}

DatasetView DatasetView::operator/=(const DataArrayConstView &other) const {
  return apply_with_delay(operator_detail::divide_equals{}, *this, other);
}

DatasetView DatasetView::operator+=(const VariableConstView &other) const {
  return apply_with_delay(operator_detail::plus_equals{}, *this, other);
}

DatasetView DatasetView::operator-=(const VariableConstView &other) const {
  return apply_with_delay(operator_detail::minus_equals{}, *this, other);
}

DatasetView DatasetView::operator*=(const VariableConstView &other) const {
  return apply_with_delay(operator_detail::times_equals{}, *this, other);
}

DatasetView DatasetView::operator/=(const VariableConstView &other) const {
  return apply_with_delay(operator_detail::divide_equals{}, *this, other);
}

DatasetView DatasetView::operator+=(const DatasetConstView &other) const {
  return apply(operator_detail::plus_equals{}, *this, other);
}

DatasetView DatasetView::operator-=(const DatasetConstView &other) const {
  return apply(operator_detail::minus_equals{}, *this, other);
}

DatasetView DatasetView::operator*=(const DatasetConstView &other) const {
  return apply(operator_detail::times_equals{}, *this, other);
}

DatasetView DatasetView::operator/=(const DatasetConstView &other) const {
  return apply(operator_detail::divide_equals{}, *this, other);
}

DatasetView DatasetView::operator+=(const Dataset &other) const {
  return apply(operator_detail::plus_equals{}, *this, other);
}

DatasetView DatasetView::operator-=(const Dataset &other) const {
  return apply(operator_detail::minus_equals{}, *this, other);
}

DatasetView DatasetView::operator*=(const Dataset &other) const {
  return apply(operator_detail::times_equals{}, *this, other);
}

DatasetView DatasetView::operator/=(const Dataset &other) const {
  return apply(operator_detail::divide_equals{}, *this, other);
}

Dataset operator+(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const Dataset &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const Dataset &lhs, const DataArrayConstView &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DatasetConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DatasetConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DatasetConstView &lhs, const DataArrayConstView &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DataArrayConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DataArrayConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const Dataset &lhs, const VariableConstView &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const VariableConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DatasetConstView &lhs, const VariableConstView &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const VariableConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const DataArrayConstView &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DatasetConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DatasetConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DatasetConstView &lhs, const DataArrayConstView &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DataArrayConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DataArrayConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const VariableConstView &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const VariableConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DatasetConstView &lhs, const VariableConstView &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const VariableConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const DataArrayConstView &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DatasetConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DatasetConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DatasetConstView &lhs, const DataArrayConstView &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DataArrayConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DataArrayConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const VariableConstView &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const VariableConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DatasetConstView &lhs, const VariableConstView &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const VariableConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const DataArrayConstView &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DatasetConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DatasetConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DatasetConstView &lhs, const DataArrayConstView &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DataArrayConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DataArrayConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const VariableConstView &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const VariableConstView &lhs, const Dataset &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DatasetConstView &lhs, const VariableConstView &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const VariableConstView &lhs, const DatasetConstView &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

} // namespace scipp::core
