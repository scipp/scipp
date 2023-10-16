// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/arithmetic.h"
#include "scipp/dataset/dataset.h"
#include "scipp/dataset/except.h"
#include "scipp/dataset/util.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

#include "dataset_operations_common.h"

using namespace scipp::core;

namespace scipp::dataset {

namespace {

template <class T, class Op> void dry_run_op(T &&a, const Variable &b, Op op) {
  // This dry run relies on the knowledge that the implementation of operations
  // for variable simply calls transform_in_place and nothing else.
  // TODO use proper op name here once dataset ops are generated
  variable::dry_run::transform_in_place(a.data(), b, op, "binary_arithmetic");
}

template <class T, class Op> void dry_run_op(T &&a, const DataArray &b, Op op) {
  expect::coords_are_superset(a, b, "");
  dry_run_op(a, b.data(), op);
}

template <class Op, class A, class B>
auto &apply(const Op &op, A &a, const B &b) {
  for (const auto &item : b)
    dry_run_op(a[item.name()], item, op);
  for (const auto &item : b)
    op(a[item.name()], item);
  return a;
}

template <typename T> bool are_same(const T &a, const T &b) {
  return a.get() == b.get();
}

template <class A, class B>
bool have_common_underlying(const A &a, const B &b) {
  return are_same(a.data_handle(), b.data_handle());
}

template <>
bool have_common_underlying<DataArray, Variable>(const DataArray &a,
                                                 const Variable &b) {
  return are_same(a.data().data_handle(), b.data_handle());
}

template <>
bool have_common_underlying<DataArray, DataArray>(const DataArray &a,
                                                  const DataArray &b) {
  return are_same(a.data().data_handle(), b.data().data_handle());
}

template <class Op, class A, class B>
decltype(auto) apply_with_delay(const Op &op, A &&a, const B &b) {
  for (auto &&item : a)
    dry_run_op(item, b, op);
  // For `b` referencing data in `a` we delay operation. The alternative would
  // be to make a deep copy of `other` before starting the iteration over items.
  DataArray delayed;
  // Note the inefficiency here: We are comparing some or all of the coords for
  // each item. This could be improved by implementing the operations for
  // internal items of Dataset instead of DataArray.
  for (auto &&item : a) {
    if (have_common_underlying(item, b))
      delayed = item;
    else
      op(item, b);
  }
  if (delayed.is_valid())
    op(delayed, b);
  return std::forward<A>(a);
}

template <class Op, class A, class B>
auto apply_with_broadcast(const Op &op, const A &a, const B &b) {
  Dataset res;
  for (const auto &item : b)
    if (const auto it = a.find(item.name()); it != a.end())
      res.setDataInit(item.name(), op(*it, item));
  return std::move(res).or_empty();
}

template <class Op, class A>
auto apply_with_broadcast(const Op &op, const A &a, const DataArray &b) {
  Dataset res;
  for (const auto &item : a)
    res.setDataInit(item.name(), op(item, b));
  return std::move(res).or_empty();
}

template <class Op, class B>
auto apply_with_broadcast(const Op &op, const DataArray &a, const B &b) {
  Dataset res;
  for (const auto &item : b)
    res.setDataInit(item.name(), op(a, item));
  return std::move(res).or_empty();
}

template <class Op, class A>
auto apply_with_broadcast(const Op &op, const A &a, const Variable &b) {
  Dataset res;
  for (const auto &item : a)
    res.setDataInit(item.name(), op(item, b));
  return std::move(res).or_empty();
}

template <class Op, class B>
auto apply_with_broadcast(const Op &op, const Variable &a, const B &b) {
  Dataset res;
  for (const auto &item : b)
    res.setDataInit(item.name(), op(a, item));
  return std::move(res).or_empty();
}

} // namespace

Dataset &Dataset::operator+=(const DataArray &other) {
  return apply_with_delay(core::element::add_equals, *this, other);
}

Dataset &Dataset::operator-=(const DataArray &other) {
  return apply_with_delay(core::element::subtract_equals, *this, other);
}

Dataset &Dataset::operator*=(const DataArray &other) {
  return apply_with_delay(core::element::multiply_equals, *this, other);
}

Dataset &Dataset::operator/=(const DataArray &other) {
  return apply_with_delay(core::element::divide_equals, *this, other);
}

Dataset &Dataset::operator+=(const Variable &other) {
  return apply_with_delay(core::element::add_equals, *this, other);
}

Dataset &Dataset::operator-=(const Variable &other) {
  return apply_with_delay(core::element::subtract_equals, *this, other);
}

Dataset &Dataset::operator*=(const Variable &other) {
  return apply_with_delay(core::element::multiply_equals, *this, other);
}

Dataset &Dataset::operator/=(const Variable &other) {
  return apply_with_delay(core::element::divide_equals, *this, other);
}

Dataset &Dataset::operator+=(const Dataset &other) {
  return apply(core::element::add_equals, *this, other);
}

Dataset &Dataset::operator-=(const Dataset &other) {
  return apply(core::element::subtract_equals, *this, other);
}

Dataset &Dataset::operator*=(const Dataset &other) {
  return apply(core::element::multiply_equals, *this, other);
}

Dataset &Dataset::operator/=(const Dataset &other) {
  return apply(core::element::divide_equals, *this, other);
}

Dataset operator+(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::add, lhs, rhs);
}

Dataset operator+(const Dataset &lhs, const DataArray &rhs) {
  return apply_with_broadcast(core::element::add, lhs, rhs);
}

Dataset operator+(const DataArray &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::add, lhs, rhs);
}

Dataset operator+(const Dataset &lhs, const Variable &rhs) {
  return apply_with_broadcast(core::element::add, lhs, rhs);
}

Dataset operator+(const Variable &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::add, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::subtract, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const DataArray &rhs) {
  return apply_with_broadcast(core::element::subtract, lhs, rhs);
}

Dataset operator-(const DataArray &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::subtract, lhs, rhs);
}

Dataset operator-(const Dataset &lhs, const Variable &rhs) {
  return apply_with_broadcast(core::element::subtract, lhs, rhs);
}

Dataset operator-(const Variable &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::subtract, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::multiply, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const DataArray &rhs) {
  return apply_with_broadcast(core::element::multiply, lhs, rhs);
}

Dataset operator*(const DataArray &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::multiply, lhs, rhs);
}

Dataset operator*(const Dataset &lhs, const Variable &rhs) {
  return apply_with_broadcast(core::element::multiply, lhs, rhs);
}

Dataset operator*(const Variable &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::multiply, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::divide, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const DataArray &rhs) {
  return apply_with_broadcast(core::element::divide, lhs, rhs);
}

Dataset operator/(const DataArray &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::divide, lhs, rhs);
}

Dataset operator/(const Dataset &lhs, const Variable &rhs) {
  return apply_with_broadcast(core::element::divide, lhs, rhs);
}

Dataset operator/(const Variable &lhs, const Dataset &rhs) {
  return apply_with_broadcast(core::element::divide, lhs, rhs);
}

} // namespace scipp::dataset
