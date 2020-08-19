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
    data().copy(slice, *this);
}

Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_unit(parent.unit()),
      m_object(parent.data().makeDefaultFromParent(dims)) {}

Variable::Variable(const VariableConstView &parent, const Dimensions &dims)
    : m_unit(parent.unit()),
      m_object(parent.underlying().data().makeDefaultFromParent(dims)) {}

Variable::Variable(const Variable &parent, VariableConceptHandle data)
    : m_unit(parent.unit()), m_object(std::move(data)) {}

VariableConstView::VariableConstView(const Variable &variable,
                                     const Dimensions &dims)
    : m_variable(&variable), m_dims(dims), m_dataDims(dims) {
  // TODO implement reshape differently, not with a special constructor?
  if (m_variable->dims().volume() != dims.volume())
    throw except::DimensionError(
        "Cannot reshape to dimensions with different volume");
}

VariableConstView::VariableConstView(const VariableConstView &slice,
                                     const Dim dim, const scipp::index begin,
                                     const scipp::index end) {
  *this = slice;
  m_offset += begin * m_dataDims.offset(dim);
  if (end == -1)
    m_dims.erase(dim);
  else
    m_dims.resize(dim, end - begin);
  // See implementation of ViewIndex regarding this relabeling.
  for (const auto label : m_dataDims.labels())
    if (label != Dim::Invalid && !m_dims.contains(label))
      m_dataDims.relabel(m_dataDims.index(label), Dim::Invalid);
}

VariableView::VariableView(Variable &variable, const Dimensions &dims)
    : VariableConstView(variable, dims), m_mutableVariable(&variable) {}

VariableView::VariableView(const VariableView &slice, const Dim dim,
                           const scipp::index begin, const scipp::index end)
    : VariableConstView(slice, dim, begin, end),
      m_mutableVariable(slice.m_mutableVariable) {}

void Variable::setDims(const Dimensions &dimensions) {
  if (dimensions.volume() == m_object->dims().volume()) {
    if (dimensions != m_object->dims())
      data().m_dimensions = dimensions;
    return;
  }
  m_object = m_object->makeDefaultFromParent(dimensions);
}

bool Variable::operator==(const VariableConstView &other) const {
  if (!*this || !other)
    return static_cast<bool>(*this) == static_cast<bool>(other);
  return data().equals(*this, other);
}

bool Variable::operator!=(const VariableConstView &other) const {
  return !(*this == other);
}

template <class T> VariableView VariableView::assign(const T &other) const {
  if (*this == VariableConstView(other))
    return *this; // Self-assignment, return early.
  setUnit(other.unit());
  core::expect::equals(dims(), other.dims());
  underlying().data().copy(other, *this);
  return *this;
}

template SCIPP_VARIABLE_EXPORT VariableView
VariableView::assign(const Variable &) const;
template SCIPP_VARIABLE_EXPORT VariableView
VariableView::assign(const VariableConstView &) const;
template SCIPP_VARIABLE_EXPORT VariableView
VariableView::assign(const VariableView &) const;

bool VariableConstView::operator==(const VariableConstView &other) const {
  if (!*this || !other)
    return static_cast<bool>(*this) == static_cast<bool>(other);
  // Always use deep comparison (pointer comparison does not make sense since we
  // may be looking at a different section).
  return underlying().data().equals(*this, other);
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

VariableConstView
VariableConstView::transpose(const std::vector<Dim> &order) const {
  auto transposed(*this);
  transposed.m_dims = core::transpose(dims(), order);
  return transposed;
}

VariableView VariableView::transpose(const std::vector<Dim> &order) const {
  auto transposed(*this);
  transposed.m_dims = core::transpose(dims(), order);
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
  if (m_offset == 0 && m_dims == m_variable->dims() &&
      m_dataDims == m_variable->dims())
    m_mutableVariable->setVariances(std::move(v));
  else
    throw except::VariancesError(
        "Cannot add variances via sliced or reshaped view of Variable.");
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
