// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

void VariableFactory::emplace(const DType key,
                              std::unique_ptr<AbstractVariableMaker> maker) {
  m_makers.emplace(key, std::move(maker));
}

bool VariableFactory::contains(const DType key) const noexcept {
  return m_makers.find(key) != m_makers.end();
}
bool VariableFactory::is_bins(const VariableConstView &var) const {
  return m_makers.at(var.dtype())->is_bins();
}

Dim VariableFactory::elem_dim(const VariableConstView &var) const {
  return m_makers.at(var.dtype())->elem_dim(var);
}

DType VariableFactory::elem_dtype(const VariableConstView &var) const {
  return m_makers.at(var.dtype())->elem_dtype(var);
}

units::Unit VariableFactory::elem_unit(const VariableConstView &var) const {
  return m_makers.at(var.dtype())->elem_unit(var);
}

void VariableFactory::expect_can_set_elem_unit(const VariableView &var,
                                               const units::Unit &u) const {
  m_makers.at(var.dtype())->expect_can_set_elem_unit(var, u);
}

void VariableFactory::set_elem_unit(const VariableView &var,
                                    const units::Unit &u) const {
  m_makers.at(var.dtype())->set_elem_unit(var, u);
}

bool VariableFactory::hasVariances(const VariableConstView &var) const {
  return m_makers.at(var.dtype())->hasVariances(var);
}

Variable VariableFactory::empty_like(const VariableConstView &prototype,
                                     const std::optional<Dimensions> &shape,
                                     const VariableConstView &sizes) {
  return m_makers.at(prototype.dtype())->empty_like(prototype, shape, sizes);
}

VariableFactory &variableFactory() {
  static VariableFactory factory;
  return factory;
}

bool is_bins(const VariableConstView &var) {
  return variableFactory().is_bins(var);
}

} // namespace scipp::variable
