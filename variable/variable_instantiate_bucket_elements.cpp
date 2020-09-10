// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bucket_variable.tcc"
#include "scipp/variable/shape.h"

namespace scipp::variable {

INSTANTIATE_VARIABLE(pair_int64, std::pair<scipp::index, scipp::index>)
INSTANTIATE_BUCKET_VARIABLE(VariableView, bucket<Variable>)

auto contiguous_indices(const VariableConstView &parent,
                        const Dimensions &dims) {
  auto indices = broadcast(parent, dims);
  scipp::index size = 0;
  for (auto &range : indices.values<core::bucket_base::range_type>()) {
    range.second += size - range.first;
    range.first = size;
    size = range.second;
  }
  return std::tuple{indices, size};
}

class VariableMakerBucketVariable : public AbstractVariableMaker {
private:
  const VariableConstView bucket_parent(const VariableConstView &parent) const {
    return parent;
  }
  const VariableConstView
  bucket_parent(const VariableConstView &parent1,
                const VariableConstView &parent2) const {
    return parent1.dtype() == dtype<bucket<Variable>> ? parent1 : parent2;
  }
  template <class... Parents>
  const VariableConstView bucket_parent(const VariableConstView &parent,
                                        const Parents &... parents) const {
    return parent.dtype() == dtype<bucket<Variable>>
               ? parent
               : bucket_parent(parents...);
  }
  template <class... Parents>
  Variable create_buckets_impl(const DType elem_dtype, const Dimensions &dims,
                               const bool variances,
                               const Parents &... parents) const {
    const auto &[parentIndices, dim, buffer] =
        bucket_parent(parents...).template constituents<bucket<Variable>>();
    auto [indices, size] = contiguous_indices(parentIndices, dims);
    auto bufferDims = buffer.dims();
    bufferDims.resize(dim, size);
    return Variable{std::make_unique<DataModel<bucket<Variable>>>(
        indices, dim,
        variableFactory().create(elem_dtype, bufferDims, variances))};
  }

public:
  Variable create(const Dimensions &, const bool) const override {
    throw std::runtime_error("Cannot create Variable with buckets without "
                             "bucket paremeters obtained from parent(s).");
  }

  Variable create_buckets(const DType elem_dtype, const Dimensions &dims,
                          const bool variances,
                          const VariableConstView &parent) const override {
    return create_buckets_impl(elem_dtype, dims, variances, parent);
  }

  Variable create_buckets(const DType elem_dtype, const Dimensions &dims,
                          const bool variances,
                          const VariableConstView &parent1,
                          const VariableConstView &parent2) const override {
    return create_buckets_impl(elem_dtype, dims, variances, parent1, parent2);
  }
};

namespace {
auto register_variable_maker_bucket_Variable(
    (variableFactory().emplace(dtype<bucket<Variable>>,
                               std::make_unique<VariableMakerBucketVariable>()),
     0));
}

} // namespace scipp::variable
