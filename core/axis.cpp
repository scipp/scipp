// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/axis.h"

namespace scipp::core {

void UnalignedAccess::set(const std::string &key, Variable var) const {
  m_unaligned->insert_or_assign(key, std::move(var));
}
void UnalignedAccess::erase(const std::string &key) const {
  m_unaligned->erase(m_unaligned->find(key));
}

template <class Id, class UnalignedType>
Axis<Id, UnalignedType>::Axis(const const_view_type &view)
    : m_data(Variable(view.data())) {
  for (const auto &item : view.unaligned())
    m_unaligned.emplace(item.first, Variable(item.second));
}

template <class Id, class UnalignedType>
typename Axis<Id, UnalignedType>::unaligned_const_view_type
Axis<Id, UnalignedType>::unaligned() const {
  typename unaligned_const_view_type::holder_type items;
  for (const auto &[key, value] : m_unaligned)
    items.emplace(key, std::pair{&value, nullptr});
  return unaligned_const_view_type{std::move(items)};
}

template <class Id, class UnalignedType>
typename Axis<Id, UnalignedType>::unaligned_view_type
Axis<Id, UnalignedType>::unaligned() {
  typename unaligned_const_view_type::holder_type items;
  for (auto &&[key, value] : m_unaligned)
    items.emplace(key, std::pair{&value, &value});
  return unaligned_view_type{UnalignedAccess(this, &m_unaligned),
                             std::move(items)};
}

const UnalignedConstView &DatasetAxisConstView::unaligned() const noexcept {
  return m_unaligned;
}

const UnalignedView &DatasetAxisView::unaligned() const noexcept {
  return m_unaligned;
}

template <class Id, class UnalignedType>
void Axis<Id, UnalignedType>::rename(const Dim from, const Dim to) {
  if (hasData())
    m_data.rename(from, to);
  for (auto &item : m_unaligned)
    item.second.rename(from, to);
}

bool operator==(const DatasetAxisConstView &a, const DatasetAxisConstView &b) {
  return a.data() == b.data() && a.unaligned() == b.unaligned();
}
bool operator!=(const DatasetAxisConstView &a, const DatasetAxisConstView &b) {
  return !(a == b);
}
bool operator==(const VariableConstView &a, const DatasetAxisConstView &b) {
  return a == b.data() && b.unaligned().empty();
}
bool operator!=(const VariableConstView &a, const DatasetAxisConstView &b) {
  return !(a == b);
}
bool operator==(const DatasetAxisConstView &a, const VariableConstView &b) {
  return b == a;
}
bool operator!=(const DatasetAxisConstView &a, const VariableConstView &b) {
  return !(a == b);
}

DatasetAxisView DatasetAxisView::
operator+=(const VariableConstView &other) const {
  data() += other;
  for (const auto &item : unaligned())
    item.second += other;
  return *this;
}
DatasetAxisView DatasetAxisView::
operator-=(const VariableConstView &other) const {
  data() -= other;
  for (const auto &item : unaligned())
    item.second -= other;
  return *this;
}
DatasetAxisView DatasetAxisView::
operator*=(const VariableConstView &other) const {
  data() *= other;
  for (const auto &item : unaligned())
    item.second *= other;
  return *this;
}
DatasetAxisView DatasetAxisView::
operator/=(const VariableConstView &other) const {
  data() /= other;
  for (const auto &item : unaligned())
    item.second /= other;
  return *this;
}
DatasetAxisView DatasetAxisView::
operator+=(const DatasetAxisConstView &) const {
  throw std::runtime_error("Operations between axes not supported yet.");
}
DatasetAxisView DatasetAxisView::
operator-=(const DatasetAxisConstView &) const {
  throw std::runtime_error("Operations between axes not supported yet.");
}
DatasetAxisView DatasetAxisView::
operator*=(const DatasetAxisConstView &) const {
  throw std::runtime_error("Operations between axes not supported yet.");
}
DatasetAxisView DatasetAxisView::
operator/=(const DatasetAxisConstView &) const {
  throw std::runtime_error("Operations between axes not supported yet.");
}

DatasetAxis resize(const DatasetAxisConstView &axis, const Dim dim,
                   const scipp::index size) {
  if (!axis.unaligned().empty())
    throw except::UnalignedError("Cannot resize with unaligned data.");
  return DatasetAxis(resize(axis.data(), dim, size));
}
DatasetAxis concatenate(const DatasetAxisConstView &a,
                        const DatasetAxisConstView &b, const Dim dim) {
  if (a.unaligned().size() != b.unaligned().size())
    throw except::UnalignedError("Mismatch of unaligned content.");
  DatasetAxis out(concatenate(a.data(), b.data(), dim));
  // TODO Name mismatches should be allowed in case of DataArray -> need
  // DataArrayAxis.
  for (const auto &[key, val] : a.unaligned())
    out.unaligned().set(key, concatenate(val, b.unaligned()[key], dim));
  return out;
}

DatasetAxis copy(const DatasetAxisConstView &axis) { return DatasetAxis(axis); }

DatasetAxis flatten(const DatasetAxisConstView &, const Dim) {
  throw std::runtime_error("flatten not supported yet.");
}

} // namespace scipp::core
