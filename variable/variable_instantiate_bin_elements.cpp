// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bin_variable.tcc"
#include "scipp/variable/bins.h"

namespace scipp::variable {

INSTANTIATE_VARIABLE(pair_int64, std::pair<int64_t, int64_t>)
INSTANTIATE_VARIABLE(pair_int32, std::pair<int32_t, int32_t>)
INSTANTIATE_BIN_VARIABLE(VariableView, bucket<Variable>)

template <class T> class BinVariableMakerVariable : public BinVariableMaker<T> {
private:
  Variable call_make_bins(const Variable &, const Variable &indices,
                          const Dim dim, const DType type,
                          const Dimensions &dims, const units::Unit &unit,
                          const bool variances) const override {
    // Buffer contains only variable, which is created with new dtype, no
    // information to copy from parent.
    return make_bins(copy(indices), dim,
                     variableFactory().create(type, dims, unit, variances));
  }
  const Variable &data(const Variable &var) const override {
    return this->buffer(var);
  }
  Variable data(Variable &var) const override { return this->buffer(var); }
};

namespace {
auto register_variable_maker_bucket_Variable(
    (variableFactory().emplace(
         dtype<bucket<Variable>>,
         std::make_unique<BinVariableMakerVariable<Variable>>()),
     0));
}

} // namespace scipp::variable
