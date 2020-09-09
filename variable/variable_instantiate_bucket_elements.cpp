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
    // transform indices of all parents?
    // how to handle and detect gaps?
    // "regularize", all that is needed are sizes
    // how to get indices from parent, if it is a view, i.e., has no data()?
    auto parentIndices = transform<bucket<Variable>::range_type>(
        parent, [](const auto &x) { return x; });
    auto indices = parentIndices;
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
    const auto volume = bufferDims.volume();

    Variable buffer;
    if (variances)
      buffer = Variable(elem_dtype, bufferDims,
                        Values(volume, core::default_init_elements),
                        Variances(volume, core::default_init_elements));
    else
      buffer = Variable(elem_dtype, bufferDims,
                        Values(volume, core::default_init_elements));

    /*
    const auto &indices = parent.data().indices();
    if (dims != parent.dims())
      throw std::runtime_error("xxxxxxxxxx");
      */
    auto tmp = indices.values<bucket<Variable>::range_type>();
    return Variable{std::make_unique<DataModel<bucket<Variable>>>(
        dims,
        element_array<typename bucket<Variable>::range_type>(tmp.begin(),
                                                             tmp.end()),
        dim, std::move(buffer))};
    return Variable(parent);
    // Need to:
    // - create buffer with new dtype and optional variances
    // - potentially broadcast buffer (translating from bucket dims to buffer
    // dims)
    // - support dtype not in scipp::variable -> AbstractVariableMaker
    // - copy coords and masks for data arrays
    // - copy bucket indices, taking into account potential broadcast
    // - handle slices

    // create(parents..., dtype, dims, variances)
    // must call maker with parent type, but somehow pass desired elem type?
    // factory must be called with parent dtype,
    // must be called with element dtype...?
    // Need:
    // - buffer size
    // - slicing dim
    // - indices
    // => all from aprent?
    // const auto volume = dims.volume();
    // if (variances)
    //  return makeVariable<T>(dims, Values(volume,
    //  core::default_init_elements),
    //                         Variances(volume, core::default_init_elements));
    // else
    //  return makeVariable<T>(dims, Values(volume,
    //  core::default_init_elements));
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
