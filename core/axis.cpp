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
  if constexpr (std::is_same_v<unaligned_type, Variable>)
    m_unaligned = Variable(view.unaligned());
  else
    for (const auto &item : view.unaligned())
      m_unaligned.emplace(item.first, Variable(item.second));
}

template <class Id, class UnalignedType>
typename Axis<Id, UnalignedType>::unaligned_const_view_type
Axis<Id, UnalignedType>::unaligned() const {
  if constexpr (std::is_same_v<unaligned_type, Variable>) {
    return m_unaligned;
  } else {
    typename unaligned_const_view_type::holder_type items;
    for (const auto &[key, value] : m_unaligned)
      items.emplace(key, VariableView(VariableConstView(value)));
    return unaligned_const_view_type{std::move(items)};
  }
}

template <class Id, class UnalignedType>
typename Axis<Id, UnalignedType>::unaligned_view_type
Axis<Id, UnalignedType>::unaligned() {
  if constexpr (std::is_same_v<unaligned_type, Variable>) {
    return m_unaligned;
  } else {
    typename unaligned_const_view_type::holder_type items;
    for (auto &&[key, value] : m_unaligned)
      items.emplace(key, VariableView(value));
    return unaligned_view_type{UnalignedAccess(this, &m_unaligned),
                               std::move(items)};
  }
}

template <class Axis>
typename AxisConstView<Axis>::unaligned_view_type
AxisConstView<Axis>::make_unaligned(
    unaligned_const_view_type const_view) const {
  if constexpr (std::is_same_v<unaligned_type, Variable>)
    return unaligned_view_type(std::move(const_view));
  else
    return unaligned_view_type(UnalignedAccess{}, std::move(const_view));
}

template <class Axis>
typename AxisConstView<Axis>::unaligned_view_type
AxisConstView<Axis>::make_empty_unaligned() const {
  if constexpr (std::is_same_v<unaligned_type, Variable>)
    return unaligned_view_type();
  else
    return unaligned_view_type(UnalignedAccess{},
                               typename UnalignedConstView::holder_type{});
}

template <class Axis>
AxisConstView<Axis>::AxisConstView(const value_type &axis)
    : m_data(axis.m_data), m_unaligned(make_unaligned(axis.unaligned())) {}

/// Constructor used by DatasetAxisView
template <class Axis>
AxisConstView<Axis>::AxisConstView(VariableView &&data,
                                   unaligned_view_type &&view)
    : m_data(std::move(data)), m_unaligned(std::move(view)) {}

// Implicit conversion from VariableConstView useful for operators.
template <class Axis>
AxisConstView<Axis>::AxisConstView(VariableConstView &&data)
    : m_data(std::move(data)), m_unaligned(make_empty_unaligned()) {}

template <class Axis>
const typename AxisConstView<Axis>::unaligned_const_view_type &
AxisConstView<Axis>::unaligned() const noexcept {
  return m_unaligned;
}

template <class Axis>
const typename AxisView<Axis>::unaligned_view_type &
AxisView<Axis>::unaligned() const noexcept {
  return AxisConstView<Axis>::m_unaligned;
}

/// Return untyped const view for data (values and optional variances).
template <class Axis> VariableConstView AxisConstView<Axis>::data() const {
  if (hasData())
    return m_data;
  throw except::SparseDataError("No data in item.");
}

/// Return untyped view for data (values and optional variances).
template <class Axis> VariableView AxisView<Axis>::data() const {
  if (AxisConstView<Axis>::hasData())
    return AxisConstView<Axis>::m_data;
  throw except::SparseDataError("No data in item.");
}

template <class Id, class UnalignedType>
void Axis<Id, UnalignedType>::rename(const Dim from, const Dim to) {
  if (hasData())
    m_data.rename(from, to);
  if constexpr (std::is_same_v<unaligned_type, Variable>) {
    m_unaligned.rename(from, to);
  } else {
    for (auto &item : m_unaligned)
      item.second.rename(from, to);
  }
}

bool operator==(const DataArrayAxisConstView &a,
                const DataArrayAxisConstView &b) {
  if (a.hasData() != b.hasData() || a.hasUnaligned() != b.hasUnaligned())
    return false;
  return (!a.hasData() || a.data() == b.data()) &&
         (!a.hasUnaligned() || a.unaligned() == b.unaligned());
}
bool operator!=(const DataArrayAxisConstView &a,
                const DataArrayAxisConstView &b) {
  return !(a == b);
}
bool operator==(const DatasetAxisConstView &a, const DatasetAxisConstView &b) {
  if (a.hasData() != b.hasData() || a.hasUnaligned() != b.hasUnaligned())
    return false;
  return (!a.hasData() || a.data() == b.data()) &&
         (!a.hasUnaligned() || a.unaligned() == b.unaligned());
}
bool operator!=(const DatasetAxisConstView &a, const DatasetAxisConstView &b) {
  return !(a == b);
}

bool operator==(const VariableConstView &a, const DataArrayAxisConstView &b) {
  if (!b.hasData())
    return false;
  return a == b.data() && !b.hasUnaligned();
}
bool operator!=(const VariableConstView &a, const DataArrayAxisConstView &b) {
  return !(a == b);
}
bool operator==(const DataArrayAxisConstView &a, const VariableConstView &b) {
  return b == a;
}
bool operator!=(const DataArrayAxisConstView &a, const VariableConstView &b) {
  return !(a == b);
}

bool operator==(const VariableConstView &a, const DatasetAxisConstView &b) {
  if (!b.hasData())
    return false;
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

template <class Axis>
AxisView<Axis> AxisView<Axis>::
operator+=(const VariableConstView &other) const {
  data() += other;
  if constexpr (std::is_same_v<unaligned_type, Variable>)
    unaligned() += other;
  else
    for (const auto &item : unaligned())
      item.second += other;
  return *this;
}
template <class Axis>
AxisView<Axis> AxisView<Axis>::
operator-=(const VariableConstView &other) const {
  data() -= other;
  if constexpr (std::is_same_v<unaligned_type, Variable>)
    unaligned() -= other;
  else
    for (const auto &item : unaligned())
      item.second -= other;
  return *this;
}
template <class Axis>
AxisView<Axis> AxisView<Axis>::
operator*=(const VariableConstView &other) const {
  data() *= other;
  if constexpr (std::is_same_v<unaligned_type, Variable>)
    unaligned() *= other;
  else
    for (const auto &item : unaligned())
      item.second *= other;
  return *this;
}
template <class Axis>
AxisView<Axis> AxisView<Axis>::
operator/=(const VariableConstView &other) const {
  data() /= other;
  if constexpr (std::is_same_v<unaligned_type, Variable>)
    unaligned() /= other;
  else
    for (const auto &item : unaligned())
      item.second /= other;
  return *this;
}
template <class Axis>
AxisView<Axis> AxisView<Axis>::operator+=(const AxisConstView<Axis> &) const {
  throw std::runtime_error("Operations between axes not supported yet.");
}
template <class Axis>
AxisView<Axis> AxisView<Axis>::operator-=(const AxisConstView<Axis> &) const {
  throw std::runtime_error("Operations between axes not supported yet.");
}
template <class Axis>
AxisView<Axis> AxisView<Axis>::operator*=(const AxisConstView<Axis> &) const {
  throw std::runtime_error("Operations between axes not supported yet.");
}
template <class Axis>
AxisView<Axis> AxisView<Axis>::operator/=(const AxisConstView<Axis> &) const {
  throw std::runtime_error("Operations between axes not supported yet.");
}

DataArrayAxis resize(const DataArrayAxisConstView &axis, const Dim dim,
                     const scipp::index size) {
  if (axis.hasUnaligned())
    throw except::UnalignedError("Cannot resize with unaligned data.");
  return DataArrayAxis(resize(axis.data(), dim, size));
}

DatasetAxis resize(const DatasetAxisConstView &axis, const Dim dim,
                   const scipp::index size) {
  if (axis.hasUnaligned())
    throw except::UnalignedError("Cannot resize with unaligned data.");
  return DatasetAxis(resize(axis.data(), dim, size));
}

DataArrayAxis concatenate(const DataArrayAxisConstView &a,
                          const DataArrayAxisConstView &b, const Dim dim) {
  return DataArrayAxis(a);
  if (static_cast<bool>(a.unaligned()) != static_cast<bool>(b.unaligned()))
    throw except::UnalignedError("Mismatch of unaligned content.");
  if(a.unaligned())
    return {concatenate(a.data(), b.data(), dim),
            concatenate(a.unaligned(), b.unaligned(), dim)};
  else
    return DataArrayAxis{concatenate(a.data(), b.data(), dim)};
}
DatasetAxis concatenate(const DatasetAxisConstView &a,
                        const DatasetAxisConstView &b, const Dim dim) {
  return DatasetAxis(a);
  if (a.unaligned().size() != b.unaligned().size())
    throw except::UnalignedError("Mismatch of unaligned content.");
  DatasetAxis out(concatenate(a.data(), b.data(), dim));
  for (const auto &[key, val] : a.unaligned())
    out.unaligned().set(key, concatenate(val, b.unaligned()[key], dim));
  return out;
}

DataArrayAxis copy(const DataArrayAxisConstView &axis) {
  return DataArrayAxis(axis);
}
DatasetAxis copy(const DatasetAxisConstView &axis) { return DatasetAxis(axis); }

DatasetAxis flatten(const DatasetAxisConstView &, const Dim) {
  throw std::runtime_error("flatten not supported yet.");
}

template class Axis<AxisId::DataArray, Variable>;
template class Axis<AxisId::Dataset, axis::dataset_unaligned_type>;
template class AxisConstView<DataArrayAxis>;
template class AxisConstView<DatasetAxis>;
template class AxisView<DataArrayAxis>;
template class AxisView<DatasetAxis>;

} // namespace scipp::core
