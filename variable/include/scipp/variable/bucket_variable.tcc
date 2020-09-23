// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/bucket_model.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.tcc"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

template <class T>
std::tuple<VariableConstView, Dim, typename T::const_element_type>
VariableConstView::constituents() const {
  auto view = *this;
  const auto &model = requireT<const DataModel<T>>(underlying().data());
  view.m_variable = &model.indices();
  return {view, model.dim(), model.buffer()};
}

template <class T>
std::tuple<VariableConstView, Dim, typename T::element_type>
VariableView::constituents() const {
  auto view = *this;
  auto &model = requireT<DataModel<T>>(m_mutableVariable->data());
  view.m_variable = &model.indices();
  return {view, model.dim(), model.buffer()};
}

namespace {
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
}

template <class T> class BucketVariableMaker : public AbstractVariableMaker {
private:
  const VariableConstView
  bucket_parent(const scipp::span<const VariableConstView> &parents) const {
    return parents.front().dtype() == dtype<bucket<T>>
               ? parents.front()
               : bucket_parent(parents.subspan(1));
  }
  virtual Variable make_buckets(const VariableConstView &parent,
                                const VariableConstView &indices, const Dim dim,
                                const DType type, const Dimensions &dims,
                                const bool variances) const = 0;

public:
  bool is_buckets() const override { return true; }

  Variable
  create(const DType elem_dtype, const Dimensions &dims, const bool variances,
         const std::vector<VariableConstView> &parents) const override {
    const VariableConstView parent = bucket_parent(parents);
    const auto &[parentIndices, dim, buffer] = parent.constituents<bucket<T>>();
    auto [indices, size] = contiguous_indices(parentIndices, dims);
    auto bufferDims = buffer.dims();
    bufferDims.resize(dim, size);
    return make_buckets(parent, indices, dim, elem_dtype, bufferDims,
                        variances);
  }

  DType elem_dtype(const VariableConstView &var) const override {
    return std::get<2>(var.constituents<bucket<T>>()).dtype();
  }
  bool hasVariances(const VariableConstView &var) const override {
    return std::get<2>(var.constituents<bucket<T>>()).hasVariances();
  }
};

/// Macro for instantiating classes and functions required for support a new
/// bucket dtype in Variable.
#define INSTANTIATE_BUCKET_VARIABLE(name, ...)                                 \
  INSTANTIATE_VARIABLE_BASE(name, __VA_ARGS__)                                 \
  template std::tuple<VariableConstView, Dim,                                  \
                      typename __VA_ARGS__::const_element_type>                \
  VariableConstView::constituents<__VA_ARGS__>() const;

} // namespace scipp::variable
