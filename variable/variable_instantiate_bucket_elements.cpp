// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bucket_variable.tcc"
#include "scipp/variable/transform.h"

namespace scipp::variable {

INSTANTIATE_VARIABLE(pair_int64, std::pair<scipp::index, scipp::index>)
INSTANTIATE_BUCKET_VARIABLE(VariableView, bucket<Variable>)

/*
auto get() {
if (var.dtype() == dype<bucket<Variable>>) {
  const auto &model = requireT<const DataModel<bucket<Variable>>>(var.data());
  return std::tuple{model.dim(), model.buffer(), model.indices()};
} else {
  throw std::runtime_error("unreachable");
}
}
*/

class VariableMakerBucketVariable : public AbstractVariableMaker {
  Variable create(const Dimensions &, const bool) const override {
    throw std::runtime_error("Cannot create Variable with buckets without "
                             "bucket paremeters obtained from parent(s).");
  }
  Variable create_buckets(const DType elem_dtype, const Dimensions &dims,
                          const bool variances,
                          const VariableConstView &parent) const override {
    auto indices = makeVariable<bucket<Variable>::range_type>(dims);
    transform_in_place<bucket<Variable>::range_type>(
        indices, parent.indices(), [](auto &x, const auto &y) { x = y; });
    scipp::index size = 0;
    for (auto &range : indices.values<bucket<Variable>::range_type>()) {
      range.second += size - range.first;
      range.first = size;
      size = range.second;
    }
    const auto &model =
        requireT<const DataModel<bucket<Variable>>>(parent.underlying().data());
    const auto dim = model.dim();
    auto bufferDims = model.buffer().dims();
    bufferDims.resize(dim, size);

    return Variable{std::make_unique<DataModel<bucket<Variable>>>(
        indices, dim,
        variableFactory().create(elem_dtype, bufferDims, variances))};
    return Variable(parent);

  }

  Variable create_buckets(const DType elem_dtype, const Dimensions &dims,
                          const bool variances,
                          const VariableConstView &parent1,
                          const VariableConstView &parent2) const override {
    return Variable(parent1);
  }
};

namespace {
auto register_variable_maker_bucket_Variable(
    (variableFactory().emplace(dtype<bucket<Variable>>,
                               std::make_unique<VariableMakerBucketVariable>()),
     0));
}

} // namespace scipp::variable
