// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <units/units.hpp>

#include "scipp/variable/variable.h"

#include "scipp/core/dtype.h"
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/creation.h"
#include "scipp/variable/except.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable_concept.h"

namespace scipp::variable {

/// Construct from parent with same dtype, unit, and hasVariances but new dims.
///
/// In the case of bucket variables the buffer size is set to zero.
Variable::Variable(const Variable &parent, const Dimensions &dims)
    : m_dims(dims), m_strides(dims),
      m_object(parent.data().makeDefaultFromParent(dims.volume())) {}

Variable::Variable(const Dimensions &dims, VariableConceptHandle data)
    : m_dims(dims), m_strides(dims), m_object(std::move(data)) {}

Variable::Variable(const llnl::units::precise_measurement &m)
    : Variable(m.value() * units::Unit(m.units())) {}

void Variable::setDataHandle(VariableConceptHandle object) {
  if (object->size() != m_object->size())
    throw except::DimensionError("Cannot replace by model of different size.");
  m_object = object;
}

const Dimensions &Variable::dims() const {
  if (!*this)
    throw std::runtime_error("invalid variable");
  return m_dims;
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
  if (this->unit() != unit && is_slice())
    throw except::UnitError("Partial view on data of variable cannot be used "
                            "to change the unit.");
}

void Variable::setUnit(const units::Unit &unit) {
  expectCanSetUnit(unit);
  expectWritable();
  m_object->setUnit(unit);
}

bool Variable::operator==(const Variable &other) const {
  if (!*this || !other)
    return static_cast<bool>(*this) == static_cast<bool>(other);
  // Note: Not comparing strides
  return dims() == other.dims() && data().equals(*this, other);
}

bool Variable::operator!=(const Variable &other) const {
  return !(*this == other);
}

scipp::span<const scipp::index> Variable::strides() const {
  return scipp::span<const scipp::index>(&*m_strides.begin(),
                                         &*m_strides.begin() + dims().ndim());
}

core::ElementArrayViewParams Variable::array_params() const noexcept {
  // TODO Translating strides into dims, which get translated back to
  // (slightly different) strides in MultiIndex. Refactor ViewIndex and
  // MultiIndex to use strides directly.
  Dimensions dataDims;
  std::vector<std::pair<Dim, scipp::index>> ordered;
  for (scipp::index i = 0; i < dims().ndim(); ++i)
    ordered.emplace_back(dims().label(i), strides()[i]);
  std::reverse(ordered.begin(), ordered.end());
  std::stable_sort(
      ordered.begin(), ordered.end(),
      [](const auto &a, const auto &b) { return a.second < b.second; });
  if (!ordered.empty() && ordered.front().second > 1) {
    dataDims.add(Dim::X, ordered.front().second);
    dataDims.relabel(0, Dim::Invalid);
  }
  for (scipp::index i = 0; i < scipp::size(ordered); ++i) {
    const auto &[dim, stride] = ordered[i];
    dataDims.add(dim, std::max(dims()[dim], ((i == scipp::size(ordered) - 1)
                                                 ? m_object->size()
                                                 : ordered[i + 1].second)) /
                          std::max(scipp::index(1), stride));
  }
  return {m_offset, dims(), dataDims, {}};
}

Variable Variable::slice(const Slice params) const {
  core::expect::validSlice(dims(), params);
  Variable out(*this);
  const auto dim = params.dim();
  const auto begin = params.begin();
  const auto end = params.end();
  const auto index = out.m_dims.index(dim);
  out.m_offset += begin * m_strides[index];
  if (end == -1) {
    out.m_strides.erase(index);
    out.m_dims.erase(dim);
  } else
    out.m_dims.resize(dim, end - begin);
  return out;
}

void Variable::validateSlice(const Slice &s, const Variable &data) const {
  if (data.unit() != this->unit())
    throw except::UnitError("Slice has different units from the Variable");
  if (data.hasVariances() != this->hasVariances())
    throw except::VariancesError("Slice and Variable must either both have "
                                 "variances, or neither should");
  // if(data.dtype() != this->dtype())
  //  throw except::TypeError("");
}

Variable &Variable::setSlice(const Slice params, const Variable &data) {
  validateSlice(params, data);
  copy(data, slice(params));
  return *this;
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

void Variable::rename(const Dim from, const Dim to) {
  if (dims().contains(from))
    m_dims.relabel(dims().index(from), to);
}

bool Variable::is_slice() const {
  // TODO Is this condition sufficient?
  return m_offset != 0 || m_dims.volume() != data().size();
}

bool Variable::is_readonly() const noexcept { return m_readonly; }

bool Variable::is_same(const Variable &other) const noexcept {
  return std::tie(m_dims, m_strides, m_offset, m_object) ==
         std::tie(other.m_dims, other.m_strides, other.m_offset,
                  other.m_object);
}

void Variable::setVariances(const Variable &v) {
  if (is_slice())
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

Variable Variable::bin_indices() const {
  auto out{*this};
  out.m_object = data().bin_indices();
  return out;
}

Variable Variable::as_const() const {
  Variable out(*this);
  out.m_readonly = true;
  return out;
}

void Variable::expectWritable() const {
  if (m_readonly)
    throw except::VariableError("Read-only flag is set, cannot mutate data.");
}

} // namespace scipp::variable
