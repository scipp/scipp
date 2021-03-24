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
    return make_bins(Variable(indices), dim,
                     variableFactory().create(type, dims, unit, variances));
  }
  const Variable &data(const Variable &var) const override {
    return requireT<const DataModel<bucket<T>>>(var.data()).buffer();
  }
  Variable &data(Variable &var) const override {
    return requireT<DataModel<bucket<T>>>(var.data()).buffer();
  }
  core::ElementArrayViewParams
  array_params(const Variable &var) const override {
    const auto &[indices, dim, buffer] = var.constituents<bucket<T>>();
    auto params = var.array_params();
    return {0, // no offset required in buffer since access via indices
            params.dims(),
            params.dataDims(),
            {dim, buffer.dims(),
             indices.template values<scipp::index_pair>().data()}};
  }
};

namespace {
auto register_variable_maker_bucket_Variable(
    (variableFactory().emplace(
         dtype<bucket<Variable>>,
         std::make_unique<BinVariableMakerVariable<Variable>>()),
     0));
}

} // namespace scipp::variable
