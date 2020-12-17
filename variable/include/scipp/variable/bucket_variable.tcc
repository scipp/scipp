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
std::tuple<Variable, Dim, typename T::buffer_type> Variable::to_constituents() {
  Variable tmp;
  std::swap(*this, tmp);
  auto &model = requireT<DataModel<T>>(tmp.data());
  return {Variable(std::move(model.indices())), model.dim(),
          std::move(model.buffer())};
}

template <class T>
std::tuple<VariableConstView, Dim, typename T::const_element_type>
Variable::constituents() const {
  return VariableConstView(*this).constituents<T>();
}

template <class T>
std::tuple<bin_indices_t<T>, Dim, typename T::element_type>
Variable::constituents() {
  return VariableView(*this).constituents<T>();
}

template <class T>
std::tuple<VariableConstView, Dim, typename T::const_element_type>
VariableConstView::constituents() const {
  const auto &model = requireT<const DataModel<T>>(underlying().data());
  auto view = *this;
  if constexpr (is_view_v<typename T::buffer_type>) {
    // See DataModel<bucket<T>>::index_values
    view = model.indices();
    view.m_offset += m_offset;
    view.m_dims = m_dims;
  } else {
    view.m_variable = &model.indices();
  }
  return {view, model.dim(), model.buffer()};
}

template <class T>
std::tuple<bin_indices_t<T>, Dim, typename T::element_type>
VariableView::constituents() const {
  auto &model = requireT<DataModel<T>>(m_mutableVariable->data());
  if constexpr (is_view_v<typename T::buffer_type>) {
    auto view = std::get<0>(VariableConstView::constituents<T>());
    return {view, model.dim(), model.buffer()};
  } else {
    auto view = *this;
    view.m_variable = &model.indices();
    view.m_mutableVariable = &model.indices();
    return {view, model.dim(), model.buffer()};
  }
}

namespace {
auto contiguous_indices(const VariableConstView &parent,
                        const Dimensions &dims) {
  auto indices = Variable(parent, dims);
  copy(parent, indices);
  scipp::index size = 0;
  for (auto &range : indices.values<core::bucket_base::range_type>()) {
    range.second += size - range.first;
    range.first = size;
    size = range.second;
  }
  return std::tuple{indices, size};
}
} // namespace

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
                                const units::Unit &unit,
                                const bool variances) const = 0;

public:
  bool is_bins() const override { return true; }

  Variable
  create(const DType elem_dtype, const Dimensions &dims,
         const units::Unit &unit, const bool variances,
         const std::vector<VariableConstView> &parents) const override {
    const VariableConstView parent = bucket_parent(parents);
    const auto &[parentIndices, dim, buffer] = parent.constituents<bucket<T>>();
    auto [indices, size] = contiguous_indices(parentIndices, dims);
    auto bufferDims = buffer.dims();
    bufferDims.resize(dim, size);
    return make_buckets(parent, indices, dim, elem_dtype, bufferDims, unit,
                        variances);
  }

  Dim elem_dim(const VariableConstView &var) const override {
    return std::get<1>(var.constituents<bucket<T>>());
  }
  DType elem_dtype(const VariableConstView &var) const override {
    return std::get<2>(var.constituents<bucket<T>>()).dtype();
  }
  units::Unit elem_unit(const VariableConstView &var) const override {
    return std::get<2>(var.constituents<bucket<T>>()).unit();
  }
  void expect_can_set_elem_unit(const VariableView &var,
                                const units::Unit &u) const override {
    if constexpr (std::is_same_v<T, VariableConstView>)
      throw std::runtime_error("Cannot set unit via const non-owning view");
    else {
      if ((elem_unit(var) != u) && (var.dims() != var.underlying().dims()))
        throw except::UnitError("Partial view on data of variable cannot be "
                                "used to change the unit.");
    }
  }
  void set_elem_unit(const VariableView &var,
                     const units::Unit &u) const override {
    if constexpr (std::is_same_v<T, VariableConstView>)
      throw std::runtime_error("Cannot set unit via const non-owning view");
    else
      std::get<2>(var.constituents<bucket<T>>()).setUnit(u);
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
  Variable::constituents<__VA_ARGS__>() const;                                 \
  template std::tuple<bin_indices_t<__VA_ARGS__>, Dim,                         \
                      typename __VA_ARGS__::element_type>                      \
  Variable::constituents<__VA_ARGS__>();                                       \
  template std::tuple<Variable, Dim, typename __VA_ARGS__::buffer_type>        \
  Variable::to_constituents<__VA_ARGS__>();                                    \
  template std::tuple<bin_indices_t<__VA_ARGS__>, Dim,                         \
                      typename __VA_ARGS__::element_type>                      \
  VariableView::constituents<__VA_ARGS__>() const;                             \
  template std::tuple<VariableConstView, Dim,                                  \
                      typename __VA_ARGS__::const_element_type>                \
  VariableConstView::constituents<__VA_ARGS__>() const;

} // namespace scipp::variable
