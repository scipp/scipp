// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <units/units.hpp>

#include "scipp/variable/variable.h"

#include "scipp/core/dtype.h"
#include "scipp/core/except.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable_concept.h"

namespace scipp::variable {

Variable::Variable(const VariableConstView &slice)
    : Variable(empty_like(slice)) {
  data().copy(slice, *this);
}

/// Construct from parent with same dtype, unit, and hasVariances but new dims.
///
/// In the case of bucket variables the buffer size is set to zero.
Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_dims(dims), m_strides(dims),
      m_object(parent.data().makeDefaultFromParent(dims.volume())) {}

Variable::Variable(const VariableConstView &parent, const Dimensions &dims)
    : m_dims(dims), m_strides(dims),
      m_object(
          parent.underlying().data().makeDefaultFromParent(dims.volume())) {}

Variable::Variable(const VariableConstView &parent, const Dimensions &dims,
                   VariableConceptHandle data)
    : m_dims(dims), m_strides(dims), m_object(std::move(data)) {}

Variable::Variable(const Dimensions &dims, VariableConceptHandle data)
    : m_dims(dims), m_strides(dims), m_object(std::move(data)) {}

Variable::Variable(const llnl::units::precise_measurement &m)
    : Variable(m.value() * units::Unit(m.units())) {}

VariableConstView::VariableConstView(const Variable &variable,
                                     const Dimensions &dims)
    : m_variable(&variable), m_offset(variable.array_params().offset()),
      m_dims(dims), m_dataDims(variable.array_params().dataDims()) {
  // TODO implement reshape differently, not with a special constructor?
  expect_same_volume(m_variable->dims(), dims);
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
  for (const auto &label : m_dataDims.labels())
    if (label != Dim::Invalid && !m_dims.contains(label))
      m_dataDims.relabel(m_dataDims.index(label), Dim::Invalid);
}

void Variable::setDims(const Dimensions &dimensions) {
  if (dimensions.volume() == dims().volume()) {
    if (dimensions != dims()) {
      m_dims = dimensions;
      m_strides = Strides(dimensions);
    }
    return;
  }
  m_dims = dimensions;
  m_strides = Strides(dimensions);
  m_object = m_object->makeDefaultFromParent(dimensions.volume());
}

void Variable::expectCanSetUnit(const units::Unit &unit) const {
  // TODO Is this condition sufficient?
  if ((this->unit() != unit) &&
      (m_offset != 0 || dims().volume() != m_object->size()))
    throw except::UnitError("Partial view on data of variable cannot be used "
                            "to change the unit.");
}

void Variable::setUnit(const units::Unit &unit) {
  expectCanSetUnit(unit);
  m_object->setUnit(unit);
}

bool Variable::operator==(const VariableConstView &other) const {
  if (!*this || !other)
    return static_cast<bool>(*this) == static_cast<bool>(other);
  // Note: Not comparing strides
  return dims() == other.dims() &&
         data().equals(*this, other);
}

bool Variable::operator!=(const VariableConstView &other) const {
  return !(*this == other);
}

Variable &Variable::assign(const VariableConstView &other) {
  // TODO return early on self-assign
  data().copy(other, *this);
  return *this;
}

bool VariableConstView::operator==(const VariableConstView &other) const {
  if (!*this || !other)
    return static_cast<bool>(*this) == static_cast<bool>(other);
  // Always use deep comparison (pointer comparison does not make sense since we
  // may be looking at a different section).
  return dims() == other.dims() && underlying().data().equals(*this, other);
}

bool VariableConstView::operator!=(const VariableConstView &other) const {
  return !(*this == other);
}

scipp::span<const scipp::index> Variable::strides() const {
  return {m_strides.begin(), dims().ndim()};
}

core::ElementArrayViewParams Variable::array_params() const noexcept {
  // TODO Translating strides into dims, which get translated back to
  // (slightly different) strides in MultiIndex
  // TODO Does not work with transpose (strides not ordered)
  Dimensions dataDims;
  bool first = true;
  for (scipp::index i = dims().ndim() - 1; i >= 0; --i) {
    if (first && strides()[i] != 1) {
      dataDims.add(Dim::X, strides()[i]);
      dataDims.relabel(0, Dim::Invalid);
    }
    first = false;
    dataDims.add(dims().label(i),
                 std::max(dims().size(i),
                          (i == 0 ? m_object->size() : strides()[i - 1])) /
                     std::max(scipp::index(1), strides()[i]));
  }
  return {m_offset, dims(), dataDims, {}};
}

Variable Variable::slice(const Slice slice) const {
  Variable out(*this);
  const auto dim = slice.dim();
  const auto begin = slice.begin();
  const auto end = slice.end();
  const auto index = out.m_dims.index(dim);
  out.m_offset += begin * m_strides[index];
  if (end == -1) {
    out.m_strides.erase(index);
    out.m_dims.erase(dim);
  } else
    out.m_dims.resize(dim, end - begin);
  return out;
}

VariableConstView VariableConstView::slice(const Slice slice) const {
  return VariableConstView(*this, slice.dim(), slice.begin(), slice.end());
}

Variable Variable::transpose(const std::vector<Dim> &order) const {
  auto transposed(*this);
  Dimensions tmp = dims();
  for (scipp::index i = 0; i < tmp.ndim(); ++i)
    tmp.resize(i, strides()[i]);
  tmp = core::transpose(tmp, order);
  transposed.m_strides = Strides(tmp.shape());
  transposed.m_dims = core::transpose(dims(), order);
  return transposed;
}

VariableConstView
VariableConstView::transpose(const std::vector<Dim> &order) const {
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

bool VariableConstView::is_trivial() const noexcept {
  return m_variable && m_offset == 0 && m_dims == m_variable->dims() &&
         m_dataDims == m_variable->dims();
}

void Variable::rename(const Dim from, const Dim to) {
  if (dims().contains(from))
    m_dims.relabel(dims().index(from), to);
}

/// Rename dims of a view. Does NOT rename dims of the underlying variable.
void VariableConstView::rename(const Dim from, const Dim to) {
  if (dims().contains(from)) {
    m_dims.relabel(m_dims.index(from), to);
    m_dataDims.relabel(m_dataDims.index(from), to);
  }
}

void Variable::setVariances(const Variable &v) {
  // TODO Is this condition sufficient?
  if (m_offset != 0 || m_dims.volume() != data().size())
    throw except::VariancesError(
        "Cannot add variances via sliced view of Variable.");
  if (v) {
    core::expect::equals(unit(), v.unit());
    core::expect::equals(dims(), v.dims());
  }
  data().setVariances(v);
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

VariableConstView Variable::bin_indices() const { return data().bin_indices(); }

VariableConstView VariableConstView::bin_indices() const {
  auto view = *this;
  view.m_variable = &underlying().bin_indices().underlying();
  return view;
}

} // namespace scipp::variable
