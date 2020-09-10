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
  Variable make_buffer(const VariableConstView &, const VariableConstView &,
                       const DType type, const Dimensions &dims,
                       const bool variances) const override {
    // Buffer contains only variable, which is created with new dtype, no
    // information to copy from parent.
    return variableFactory().create(type, dims, variances);
  }
};

namespace {
auto register_variable_maker_bucket_Variable(
    (variableFactory().emplace(dtype<bucket<Variable>>,
                               std::make_unique<BucketVariableMakerVariable>()),
     0));
}

} // namespace scipp::variable
