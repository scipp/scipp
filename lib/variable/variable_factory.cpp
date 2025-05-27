// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

DType VariableFactory::bin_dtype(const parent_list &vars) const noexcept {
  // First pass: Find, e.g., bucket<DataArray>, since if we have such an input
  // we need to keep metadata in the output. Note that we would also like to
  // prioritize bucket<Dataset> higher than bucket<DataArray>, but the former
  // is not supported in binary operations, so this is not relevant right now.
  if (const auto it = std::find_if(vars.begin(), vars.end(),
                                   [](const Variable &v) {
                                     return variable::is_bins(v) &&
                                            v.dtype() !=
                                                dtype<bucket<Variable>>;
                                   });
      it != vars.end())
    return it->get().dtype();
  // Second pass: Find bucket<Variable> (or other)
  if (const auto it =
          std::find_if(vars.begin(), vars.end(),
                       [](const auto &v) { return variable::is_bins(v); });
      it != vars.end())
    return it->get().dtype();
  // Not binned data
  return dtype<void>;
}

void VariableFactory::emplace(const DType key,
                              std::unique_ptr<AbstractVariableMaker> maker) {
  m_makers.emplace(key, std::move(maker));
}

bool VariableFactory::contains(const DType key) const noexcept {
  return m_makers.find(key) != m_makers.end();
}
bool VariableFactory::is_bins(const Variable &var) const {
  return m_makers.at(var.dtype())->is_bins();
}

Dim VariableFactory::elem_dim(const Variable &var) const {
  return m_makers.at(var.dtype())->elem_dim(var);
}

DType VariableFactory::elem_dtype(const Variable &var) const {
  return m_makers.at(var.dtype())->elem_dtype(var);
}

sc_units::Unit VariableFactory::elem_unit(const Variable &var) const {
  return m_makers.at(var.dtype())->elem_unit(var);
}

void VariableFactory::expect_can_set_elem_unit(const Variable &var,
                                               const sc_units::Unit &u) const {
  m_makers.at(var.dtype())->expect_can_set_elem_unit(var, u);
}

void VariableFactory::set_elem_unit(Variable &var,
                                    const sc_units::Unit &u) const {
  m_makers.at(var.dtype())->set_elem_unit(var, u);
}

bool VariableFactory::has_masks(const Variable &var) const {
  return m_makers.at(var.dtype())->has_masks(var);
}

bool VariableFactory::has_variances(const Variable &var) const {
  return m_makers.at(var.dtype())->has_variances(var);
}

Variable VariableFactory::empty_like(const Variable &prototype,
                                     const std::optional<Dimensions> &shape,
                                     const Variable &sizes) {
  return m_makers.at(prototype.dtype())->empty_like(prototype, shape, sizes);
}

Variable VariableFactory::apply_event_masks(const Variable &var,
                                            const FillValue fill) const {
  return m_makers.at(var.dtype())->apply_event_masks(var, fill);
}

Variable VariableFactory::irreducible_event_mask(const Variable &var) const {
  return m_makers.at(var.dtype())->irreducible_event_mask(var);
}

VariableFactory &variableFactory() {
  static VariableFactory factory;
  return factory;
}

bool is_bins(const Variable &var) { return variableFactory().is_bins(var); }

} // namespace scipp::variable
