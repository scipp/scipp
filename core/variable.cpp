// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/variable.h"
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"

namespace scipp::core {

std::vector<scipp::index>
detail::reorderedShape(const scipp::span<const Dim> &order,
                       const Dimensions &dimensions) {
  if (order.size() != dimensions.ndim())
    throw std::runtime_error("Cannot transpose input dimensions should be "
                             "exactly the same but maybe in different order.");
  std::vector<scipp::index> res(order.size());
  std::transform(order.begin(), order.end(), res.begin(),
                 [&dimensions](auto &a) { return dimensions[a]; });
  return res;
}

template <class... Known>
VariableConceptHandle_impl<Known...>::operator bool() const noexcept {
  return std::visit([](auto &&ptr) { return bool(ptr); }, m_object);
}

template <class... Known>
VariableConcept &VariableConceptHandle_impl<Known...>::operator*() const {
  return std::visit([](auto &&arg) -> VariableConcept & { return *arg; },
                    m_object);
}

template <class... Known>
VariableConcept *VariableConceptHandle_impl<Known...>::operator->() const {
  return std::visit(
      [](auto &&arg) -> VariableConcept * { return arg.operator->(); },
      m_object);
}

template <class... Known>
typename VariableConceptHandle_impl<Known...>::variant_t
VariableConceptHandle_impl<Known...>::variant() const noexcept {
  return std::visit(
      [](auto &&arg) {
        return std::variant<const VariableConcept *,
                            const VariableConceptT<Known> *...>{arg.get()};
      },
      m_object);
}

// Explicit instantiation of complete class does not work, at least on gcc.
// Apparently the type is already defined and the attribute is ignored, so we
// have to do it separately for each method.
template SCIPP_CORE_EXPORT VariableConceptHandle_impl<KNOWN>::
operator bool() const;
template SCIPP_CORE_EXPORT VariableConcept &VariableConceptHandle_impl<KNOWN>::
operator*() const;
template SCIPP_CORE_EXPORT VariableConcept *VariableConceptHandle_impl<KNOWN>::
operator->() const;
template SCIPP_CORE_EXPORT typename VariableConceptHandle_impl<KNOWN>::variant_t
VariableConceptHandle_impl<KNOWN>::variant() const noexcept;

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions) {}

Variable::Variable(const VariableConstView &slice)
    : Variable(slice, slice.dims()) {
  // There is a bug in the implementation of MultiIndex used in ElementArrayView
  // in case one of the dimensions has extent 0.
  if (dims().volume() != 0)
    data().copy(slice.data(), Dim::Invalid, 0, 0, 1);
}

Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_unit(parent.unit()),
      m_object(parent.m_object->makeDefaultFromParent(dims)) {}

Variable::Variable(const VariableConstView &parent, const Dimensions &dims)
    : m_unit(parent.unit()),
      m_object(parent.data().makeDefaultFromParent(dims)) {}

Variable::Variable(const Variable &parent, VariableConceptHandle data)
    : m_unit(parent.unit()), m_object(std::move(data)) {}

void Variable::setDims(const Dimensions &dimensions) {
  if (dimensions.volume() == m_object->dims().volume()) {
    if (dimensions != m_object->dims())
      data().m_dimensions = dimensions;
    return;
  }
  m_object = m_object->makeDefaultFromParent(dimensions);
}

template <class T1, class T2> bool equals(const T1 &a, const T2 &b) {
  if (!a || !b)
    return static_cast<bool>(a) == static_cast<bool>(b);
  if (a.unit() != b.unit())
    return false;
  return a.data() == b.data();
}

bool Variable::operator==(const VariableConstView &other) const {
  return equals(*this, other);
}

bool Variable::operator!=(const VariableConstView &other) const {
  return !(*this == other);
}

template <class T> VariableView VariableView::assign(const T &other) const {
  if (data().isSame(other.data()))
    return *this; // Self-assignment, return early.
  setUnit(other.unit());
  expect::equals(dims(), other.dims());
  data().copy(other.data(), Dim::Invalid, 0, 0, 1);
  return *this;
}

template SCIPP_CORE_EXPORT VariableView
VariableView::assign(const Variable &) const;
template SCIPP_CORE_EXPORT VariableView
VariableView::assign(const VariableConstView &) const;
template SCIPP_CORE_EXPORT VariableView
VariableView::assign(const VariableView &) const;

bool VariableConstView::operator==(const VariableConstView &other) const {
  // Always use deep comparison (pointer comparison does not make sense since we
  // may be looking at a different section).
  return equals(*this, other);
}

bool VariableConstView::operator!=(const VariableConstView &other) const {
  return !(*this == other);
}

void VariableView::setUnit(const units::Unit &unit) const {
  expectCanSetUnit(unit);
  m_mutableVariable->setUnit(unit);
}

void VariableView::expectCanSetUnit(const units::Unit &unit) const {
  if ((this->unit() != unit) && (dims() != m_mutableVariable->dims()))
    throw except::UnitError("Partial view on data of variable cannot be used "
                            "to change the unit.");
}

VariableConstView Variable::slice(const Slice slice) const & {
  return {*this, slice.dim(), slice.begin(), slice.end()};
}

Variable Variable::slice(const Slice slice) const && {
  return Variable{this->slice(slice)};
}

VariableView Variable::slice(const Slice slice) & {
  return {*this, slice.dim(), slice.begin(), slice.end()};
}

Variable Variable::slice(const Slice slice) && {
  return Variable{this->slice(slice)};
}

VariableConstView Variable::reshape(const Dimensions &dims) const & {
  return {*this, dims};
}

VariableView Variable::reshape(const Dimensions &dims) & {
  return {*this, dims};
}

Variable Variable::reshape(const Dimensions &dims) && {
  Variable reshaped(std::move(*this));
  reshaped.setDims(dims);
  return reshaped;
}

Variable VariableConstView::reshape(const Dimensions &dims) const {
  // In general a variable slice is not contiguous. Therefore we cannot reshape
  // without making a copy (except for special cases).
  Variable reshaped(*this);
  reshaped.setDims(dims);
  return reshaped;
}

template <class DimContainer>
std::vector<Dim> reverseDimOrder(const DimContainer &container) {
  return std::vector<Dim>(container.rbegin(), container.rend());
}

VariableConstView Variable::transpose(const std::vector<Dim> &dims) const & {
  return VariableConstView::makeTransposed(
      *this, dims.empty() ? reverseDimOrder(this->dims().labels()) : dims);
}

VariableView Variable::transpose(const std::vector<Dim> &dims) & {
  return VariableView::makeTransposed(
      *this, dims.empty() ? reverseDimOrder(this->dims().labels()) : dims);
}

Variable Variable::transpose(const std::vector<Dim> &dims) && {
  return Variable(VariableConstView::makeTransposed(
      *this, dims.empty() ? reverseDimOrder(this->dims().labels()) : dims));
}

VariableConstView
VariableConstView::transpose(const std::vector<Dim> &dims) const {
  auto dms = this->dims();
  return makeTransposed(*this,
                        dims.empty() ? reverseDimOrder(dms.labels()) : dims);
}

VariableView VariableView::transpose(const std::vector<Dim> &dims) const {
  auto dms = this->dims();
  return makeTransposed(*this,
                        dims.empty() ? reverseDimOrder(dms.labels()) : dims);
}

void Variable::rename(const Dim from, const Dim to) {
  if (dims().contains(from))
    data().m_dimensions.relabel(dims().index(from), to);
}

void Variable::setVariances(Variable v) {
  if (v)
    expect::equals(unit(), v.unit());
  data().setVariances(std::move(v));
}

void VariableView::setVariances(Variable v) const {
  // If the view wraps the whole variable (common case: iterating a dataset)
  // m_view is not set. A more complex check would be to verify dimensions,
  // shape, and strides, but this should be sufficient for now.
  if (m_view)
    throw except::VariancesError(
        "Cannot add variances via sliced or reshaped view of Variable.");
  m_mutableVariable->setVariances(std::move(v));
}

namespace detail {
void throw_variance_without_value() {
  throw except::VariancesError("Can't have variance without values");
}

void throw_keyword_arg_constructor_bad_dtype(const DType dtype) {
  throw except::TypeError("Can't create the Variable with type " +
                          to_string(dtype) +
                          " with such values and/or variances.");
}

void expect0D(const Dimensions &dims) { expect::equals(dims, Dimensions()); }

} // namespace detail

} // namespace scipp::core
