// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bins.h"
#include "scipp/variable/bucket_variable.tcc"

namespace scipp::variable {

INSTANTIATE_VARIABLE(pair_int64, std::pair<int64_t, int64_t>)
INSTANTIATE_VARIABLE(pair_int32, std::pair<int32_t, int32_t>)
INSTANTIATE_BUCKET_VARIABLE(VariableView, bucket<Variable>)
INSTANTIATE_BUCKET_VARIABLE(VariableView_observer, bucket<VariableView>)
INSTANTIATE_BUCKET_VARIABLE(VariableConstView_observer,
                            bucket<VariableConstView>)

template <class T> class BinVariableMakerVariable : public BinVariableMaker<T> {
private:
  Variable call_make_bins(const VariableConstView &,
                          const VariableConstView &indices, const Dim dim,
                          const DType type, const Dimensions &dims,
                          const units::Unit &unit,
                          const bool variances) const override {
    // Buffer contains only variable, which is created with new dtype, no
    // information to copy from parent.
    return make_bins(Variable(indices), dim,
                     variableFactory().create(type, dims, unit, variances));
  }
  VariableConstView data(const VariableConstView &var) const override {
    return std::get<2>(var.constituents<bucket<T>>());
  }
  VariableView data(const VariableView &var) const override {
    if constexpr (std::is_same_v<T, VariableConstView>)
      // This code is an indication of some shortcomings with the const handling
      // of variables and views. Essentially we would require better support for
      // variables with const elements.
      throw std::runtime_error("Mutable access to data of non-owning binned "
                               "view of const buffer is not possible.");
    else
      return std::get<2>(var.constituents<bucket<T>>());
  }
  core::ElementArrayViewParams
  array_params(const VariableConstView &var) const override {
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
     variableFactory().emplace(
         dtype<bucket<VariableView>>,
         std::make_unique<BinVariableMakerVariable<VariableView>>()),
     variableFactory().emplace(
         dtype<bucket<VariableConstView>>,
         std::make_unique<BinVariableMakerVariable<VariableConstView>>()),
     0));
}

} // namespace scipp::variable
