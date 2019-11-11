// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/variable.h"
#include "scipp/core/counts.h"
#include "scipp/core/dtype.h"
#include "scipp/core/except.h"

namespace scipp::core {

VariableConcept::VariableConcept(const Dimensions &dimensions)
    : m_dimensions(dimensions) {}

Variable::Variable(const VariableConstProxy &slice)
    : Variable(*slice.m_variable) {
  if (slice.m_view) {
    setUnit(slice.unit());
    setDims(slice.dims());
    // There is a bug in the implementation of MultiIndex used in VariableView
    // in case one of the dimensions has extent 0.
    if (dims().volume() != 0)
      data().copy(slice.data(), Dim::Invalid, 0, 0, 1);
  }
}

Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_unit(parent.unit()),
      m_object(parent.m_object->makeDefaultFromParent(dims)) {}

Variable::Variable(const VariableConstProxy &parent, const Dimensions &dims)
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

bool Variable::operator==(const VariableConstProxy &other) const {
  return equals(*this, other);
}

bool Variable::operator!=(const VariableConstProxy &other) const {
  return !(*this == other);
}

template <class T> VariableProxy VariableProxy::assign(const T &other) const {
  setUnit(other.unit());
  expect::equals(dims(), other.dims());
  data().copy(other.data(), Dim::Invalid, 0, 0, 1);
  return *this;
}

template SCIPP_CORE_EXPORT VariableProxy
VariableProxy::assign(const Variable &) const;
template SCIPP_CORE_EXPORT VariableProxy
VariableProxy::assign(const VariableConstProxy &) const;
template SCIPP_CORE_EXPORT VariableProxy
VariableProxy::assign(const VariableProxy &) const;

bool VariableConstProxy::operator==(const VariableConstProxy &other) const {
  // Always use deep comparison (pointer comparison does not make sense since we
  // may be looking at a different section).
  return equals(*this, other);
}

bool VariableConstProxy::operator!=(const VariableConstProxy &other) const {
  return !(*this == other);
}

void VariableProxy::setUnit(const units::Unit &unit) const {
  expectCanSetUnit(unit);
  m_mutableVariable->setUnit(unit);
}

void VariableProxy::expectCanSetUnit(const units::Unit &unit) const {
  if ((this->unit() != unit) && (dims() != m_mutableVariable->dims()))
    throw except::UnitError("Partial view on data of variable cannot be used "
                            "to change the unit.");
}

VariableConstProxy Variable::slice(const Slice slice) const & {
  return {*this, slice.dim(), slice.begin(), slice.end()};
}

Variable Variable::slice(const Slice slice) const && {
  return Variable{this->slice(slice)};
}

VariableProxy Variable::slice(const Slice slice) & {
  return {*this, slice.dim(), slice.begin(), slice.end()};
}

Variable Variable::slice(const Slice slice) && {
  return Variable{this->slice(slice)};
}

VariableConstProxy Variable::reshape(const Dimensions &dims) const & {
  return {*this, dims};
}

VariableProxy Variable::reshape(const Dimensions &dims) & {
  return {*this, dims};
}

Variable Variable::reshape(const Dimensions &dims) && {
  Variable reshaped(std::move(*this));
  reshaped.setDims(dims);
  return reshaped;
}

Variable VariableConstProxy::reshape(const Dimensions &dims) const {
  // In general a variable slice is not contiguous. Therefore we cannot reshape
  // without making a copy (except for special cases).
  Variable reshaped(*this);
  reshaped.setDims(dims);
  return reshaped;
}

void Variable::rename(const Dim from, const Dim to) {
  if (dims().contains(from))
    data().m_dimensions.relabel(dims().index(from), to);
}

} // namespace scipp::core
