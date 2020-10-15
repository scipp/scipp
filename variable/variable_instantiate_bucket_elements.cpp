// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bucket_variable.tcc"

namespace scipp::variable {

INSTANTIATE_VARIABLE(pair_int64, std::pair<scipp::index, scipp::index>)
INSTANTIATE_BUCKET_VARIABLE(VariableView, bucket<Variable>)

class BucketVariableMakerVariable : public BucketVariableMaker<Variable> {
private:
  Variable make_buckets(const VariableConstView &,
                        const VariableConstView &indices, const Dim dim,
                        const DType type, const Dimensions &dims,
                        const units::Unit &unit,
                        const bool variances) const override {
    // Buffer contains only variable, which is created with new dtype, no
    // information to copy from parent.
    return Variable{std::make_unique<DataModel<bucket<Variable>>>(
        indices, dim, variableFactory().create(type, dims, unit, variances))};
  }
  VariableConstView data(const VariableConstView &var) const override {
    return std::get<2>(var.constituents<bucket<Variable>>());
  }
  VariableView data(const VariableView &var) const override {
    return std::get<2>(var.constituents<bucket<Variable>>());
  }
  core::element_array_view
  array_params(const VariableConstView &var) const override {
    const auto &[indices, dim, buffer] = var.constituents<bucket<Variable>>();
    auto params = var.array_params();
    return {0, // no offset required in buffer since access via indices
            params.dims(),
            params.dataDims(),
            {dim, buffer.dims(),
             indices.values<std::pair<scipp::index, scipp::index>>().data()}};
  }
};

namespace {
auto register_variable_maker_bucket_Variable(
    (variableFactory().emplace(dtype<bucket<Variable>>,
                               std::make_unique<BucketVariableMakerVariable>()),
     0));
}

} // namespace scipp::variable
