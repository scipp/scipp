// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/core/transform.h"

#include "operators.h"

namespace scipp::core {

template <class Op>
void dry_run_op(const DataProxy &a, const VariableConstProxy &b, Op op) {
  // This dry run relies on the knowledge that the implementation of operations
  // for variable simply calls transform_in_place and nothing else.
  dry_run::transform_in_place(a.data(), b, op);
}

template <class Op>
void dry_run_op(const DataProxy &a, const DataConstProxy &b, Op op) {
  expect::coordsAndLabelsAreSuperset(a, b);
  dry_run_op(a, b.data(), op);
}

DataProxy DataProxy::operator+=(const DataConstProxy &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() += other.data();
  return *this;
}

DataProxy DataProxy::operator-=(const DataConstProxy &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() -= other.data();
  return *this;
}

DataProxy DataProxy::operator*=(const DataConstProxy &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() *= other.data();
  return *this;
}

DataProxy DataProxy::operator/=(const DataConstProxy &other) const {
  expect::coordsAndLabelsAreSuperset(*this, other);
  data() /= other.data();
  return *this;
}

DataProxy DataProxy::operator+=(const VariableConstProxy &other) const {
  data() += other;
  return *this;
}

DataProxy DataProxy::operator-=(const VariableConstProxy &other) const {
  data() -= other;
  return *this;
}

DataProxy DataProxy::operator*=(const VariableConstProxy &other) const {
  data() *= other;
  return *this;
}

DataProxy DataProxy::operator/=(const VariableConstProxy &other) const {
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
  for (const auto & [ name, item ] : b)
    dry_run_op(a[name], item, op);
  for (const auto & [ name, item ] : b)
    op(a[name], item);
  return a;
}

template <typename T> bool are_same(const T &a, const T &b) { return &a == &b; }

template <class A, class B>
bool have_common_underlying(const A &a, const B &b) {
  return are_same(a.underlying(), b.underlying());
}

template <>
bool have_common_underlying<DataProxy, VariableConstProxy>(
    const DataProxy &a, const VariableConstProxy &b) {
  return are_same(*a.underlying().data, b.underlying());
}

template <class Op, class A, class B>
decltype(auto) apply_with_delay(const Op &op, A &&a, const B &b) {
  for (const auto &item : a)
    dry_run_op(item.second, b, op);
  // For `b` referencing data in `a` we delay operation. The alternative would
  // be to make a deep copy of `other` before starting the iteration over items.
  std::optional<DataProxy> delayed;
  // Note the inefficiency here: We are comparing some or all of the coords and
  // labels for each item. This could be improved by implementing the operations
  // for detail::DatasetData instead of DataProxy.
  for (const auto & [ name, item ] : a) {
    static_cast<void>(name);
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
  for (const auto & [ name, item ] : b)
    if (const auto it = a.find(name); it != a.end())
      res.setData(std::string(name), op(it->second, item));
  return res;
}

template <class Op, class A>
auto apply_with_broadcast(const Op &op, const A &a, const DataConstProxy &b) {
  Dataset res;
  for (const auto & [ name, item ] : a)
    res.setData(std::string(name), op(item, b));
  return res;
}

template <class Op, class B>
auto apply_with_broadcast(const Op &op, const DataConstProxy &a, const B &b) {
  Dataset res;
  for (const auto & [ name, item ] : b)
    res.setData(std::string(name), op(a, item));
  return res;
}

template <class Op, class A>
auto apply_with_broadcast(const Op &op, const A &a,
                          const VariableConstProxy &b) {
  Dataset res;
  for (const auto & [ name, item ] : a)
    res.setData(std::string(name), op(item, b));
  return res;
}

template <class Op, class B>
auto apply_with_broadcast(const Op &op, const VariableConstProxy &a,
                          const B &b) {
  Dataset res;
  for (const auto & [ name, item ] : b)
    res.setData(std::string(name), op(a, item));
  return res;
}

Dataset &Dataset::operator+=(const DataConstProxy &other) {
  return apply_with_delay(operator_detail::plus_equals{}, *this, other);
}

Dataset &Dataset::operator-=(const DataConstProxy &other) {
  return apply_with_delay(operator_detail::minus_equals{}, *this, other);
}

Dataset &Dataset::operator*=(const DataConstProxy &other) {
  return apply_with_delay(operator_detail::times_equals{}, *this, other);
}

Dataset &Dataset::operator/=(const DataConstProxy &other) {
  return apply_with_delay(operator_detail::divide_equals{}, *this, other);
}

Dataset &Dataset::operator+=(const VariableConstProxy &other) {
  return apply_with_delay(operator_detail::plus_equals{}, *this, other);
}

Dataset &Dataset::operator-=(const VariableConstProxy &other) {
  return apply_with_delay(operator_detail::minus_equals{}, *this, other);
}

Dataset &Dataset::operator*=(const VariableConstProxy &other) {
  return apply_with_delay(operator_detail::times_equals{}, *this, other);
}

Dataset &Dataset::operator/=(const VariableConstProxy &other) {
  return apply_with_delay(operator_detail::divide_equals{}, *this, other);
}

Dataset &Dataset::operator+=(const DatasetConstProxy &other) {
  return apply(operator_detail::plus_equals{}, *this, other);
}

Dataset &Dataset::operator-=(const DatasetConstProxy &other) {
  return apply(operator_detail::minus_equals{}, *this, other);
}

Dataset &Dataset::operator*=(const DatasetConstProxy &other) {
  return apply(operator_detail::times_equals{}, *this, other);
}

Dataset &Dataset::operator/=(const DatasetConstProxy &other) {
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

DatasetProxy DatasetProxy::operator+=(const DataConstProxy &other) const {
  return apply_with_delay(operator_detail::plus_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator-=(const DataConstProxy &other) const {
  return apply_with_delay(operator_detail::minus_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator*=(const DataConstProxy &other) const {
  return apply_with_delay(operator_detail::times_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator/=(const DataConstProxy &other) const {
  return apply_with_delay(operator_detail::divide_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator+=(const VariableConstProxy &other) const {
  return apply_with_delay(operator_detail::plus_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator-=(const VariableConstProxy &other) const {
  return apply_with_delay(operator_detail::minus_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator*=(const VariableConstProxy &other) const {
  return apply_with_delay(operator_detail::times_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator/=(const VariableConstProxy &other) const {
  return apply_with_delay(operator_detail::divide_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator+=(const DatasetConstProxy &other) const {
  return apply(operator_detail::plus_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator-=(const DatasetConstProxy &other) const {
  return apply(operator_detail::minus_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator*=(const DatasetConstProxy &other) const {
  return apply(operator_detail::times_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator/=(const DatasetConstProxy &other) const {
  return apply(operator_detail::divide_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator+=(const Dataset &other) const {
  return apply(operator_detail::plus_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator-=(const Dataset &other) const {
  return apply(operator_detail::minus_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator*=(const Dataset &other) const {
  return apply(operator_detail::times_equals{}, *this, other);
}

DatasetProxy DatasetProxy::operator/=(const Dataset &other) const {
  return apply(operator_detail::divide_equals{}, *this, other);
}

Dataset operator+(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const Dataset &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const Dataset &lhs, const DataConstProxy &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DatasetConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DatasetConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DatasetConstProxy &lhs, const DataConstProxy &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DataConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DataConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const Dataset &lhs, const VariableConstProxy &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const VariableConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const DatasetConstProxy &lhs, const VariableConstProxy &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator+(const VariableConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(plus, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const DataConstProxy &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DatasetConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DatasetConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DatasetConstProxy &lhs, const DataConstProxy &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DataConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DataConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const VariableConstProxy &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const VariableConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const DatasetConstProxy &lhs, const VariableConstProxy &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator-(const VariableConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(minus, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const DataConstProxy &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DatasetConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DatasetConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DatasetConstProxy &lhs, const DataConstProxy &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DataConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DataConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const VariableConstProxy &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const VariableConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const DatasetConstProxy &lhs, const VariableConstProxy &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator*(const VariableConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(times, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const DataConstProxy &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DatasetConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DatasetConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DatasetConstProxy &lhs, const DataConstProxy &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DataConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DataConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const VariableConstProxy &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const VariableConstProxy &lhs, const Dataset &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const DatasetConstProxy &lhs, const VariableConstProxy &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

Dataset operator/(const VariableConstProxy &lhs, const DatasetConstProxy &rhs) {
  return apply_with_broadcast(divide, lhs, rhs);
}

} // namespace scipp::core
