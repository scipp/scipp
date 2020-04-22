// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/variable.h"

#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/variable/variable_concept.h"

namespace scipp::variable {

Variable::Variable(const VariableConstView &slice)
    : Variable(slice ? Variable(slice, slice.dims()) : Variable()) {
  // There is a bug in the implementation of MultiIndex used in ElementArrayView
  // in case one of the dimensions has extent 0.
  if (slice && dims().volume() != 0)
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

VariableConstView::VariableConstView(const Variable &variable,
                                     const Dimensions &dims)
    : m_variable(&variable), m_view(variable.data().reshape(dims)) {}

VariableConstView::VariableConstView(const Variable &variable, const Dim dim,
                                     const scipp::index begin,
                                     const scipp::index end)
    : m_variable(&variable), m_view(variable.data().makeView(dim, begin, end)) {
}

VariableConstView::VariableConstView(const VariableConstView &slice,
                                     const Dim dim, const scipp::index begin,
                                     const scipp::index end)
    : m_variable(slice.m_variable),
      m_view(slice.data().makeView(dim, begin, end)) {}

// Note that we use the basic constructor of VariableConstView to avoid
// creation of a const m_view, which would be overwritten immediately.
VariableView::VariableView(Variable &variable, const Dimensions &dims)
    : VariableConstView(variable), m_mutableVariable(&variable) {
  m_view = variable.data().reshape(dims);
}

VariableView::VariableView(Variable &variable, const Dim dim,
                           const scipp::index begin, const scipp::index end)
    : VariableConstView(variable), m_mutableVariable(&variable) {
  m_view = variable.data().makeView(dim, begin, end);
}

VariableView::VariableView(const VariableView &slice, const Dim dim,
                           const scipp::index begin, const scipp::index end)
    : VariableConstView(slice), m_mutableVariable(slice.m_mutableVariable) {
  m_view = slice.data().makeView(dim, begin, end);
}

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
  core::expect::equals(dims(), other.dims());
  data().copy(other.data(), Dim::Invalid, 0, 0, 1);
  return *this;
}

template SCIPP_VARIABLE_EXPORT VariableView
VariableView::assign(const Variable &) const;
template SCIPP_VARIABLE_EXPORT VariableView
VariableView::assign(const VariableConstView &) const;
template SCIPP_VARIABLE_EXPORT VariableView
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

VariableConstView VariableConstView::slice(const Slice slice) const {
  return VariableConstView(*this, slice.dim(), slice.begin(), slice.end());
}

VariableView VariableView::slice(const Slice slice) const {
  return VariableView(*this, slice.dim(), slice.begin(), slice.end());
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

VariableConstView Variable::transpose(const std::vector<Dim> &dims) const & {
  return VariableConstView(*this).transpose(dims);
}

VariableView Variable::transpose(const std::vector<Dim> &dims) & {
  return VariableView(*this).transpose(dims);
}

Variable Variable::transpose(const std::vector<Dim> &dims) && {
  return Variable(VariableConstView(*this).transpose(dims));
}

VariableConstView
VariableConstView::transpose(const std::vector<Dim> &dims) const {
  auto transposed(*this);
  transposed.m_view = data().transpose(dims);
  return transposed;
}

VariableView VariableView::transpose(const std::vector<Dim> &dims) const {
  auto transposed(*this);
  transposed.m_view = data().transpose(dims);
  return transposed;
}

std::vector<scipp::index> VariableConstView::strides() const {
  const auto parent = m_variable->dims();
  std::vector<scipp::index> strides;
  for (const auto &label : parent.labels())
    if (dims().contains(label))
      strides.emplace_back(parent.offset(label));
  return strides;
}

void Variable::rename(const Dim from, const Dim to) {
  if (dims().contains(from))
    data().m_dimensions.relabel(dims().index(from), to);
}

void Variable::setVariances(Variable v) {
  if (v)
    core::expect::equals(unit(), v.unit());
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
void throw_keyword_arg_constructor_bad_dtype(const DType dtype) {
  throw except::TypeError("Can't create the Variable with type " +
                          to_string(dtype) +
                          " with such values and/or variances.");
}

void expect0D(const Dimensions &dims) {
  core::expect::equals(dims, Dimensions());
}

} // namespace detail

} // namespace scipp::variable
