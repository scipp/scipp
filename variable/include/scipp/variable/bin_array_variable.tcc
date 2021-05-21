// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/arithmetic.h"
#include "scipp/variable/bin_array_model.h"
#include "scipp/variable/cumulative.h"
#include "scipp/variable/shape.h"
#include "scipp/variable/variable.tcc"
#include "scipp/variable/variable_factory.h"

namespace scipp::variable {

extern template class ElementArrayModel<int64_t>;

template <class T> std::tuple<Variable, Dim, T> Variable::to_constituents() {
  Variable tmp;
  std::swap(*this, tmp);
  auto &model = requireT<BinArrayModel<T>>(tmp.data());
  return {tmp.bin_indices(), model.bin_dim(), std::move(model.buffer())};
}

template <class T> std::tuple<Variable, Dim, T> Variable::constituents() const {
  auto &model = requireT<const BinArrayModel<T>>(data());
  return {bin_indices(), model.bin_dim(), model.buffer()};
}

template <class T> std::tuple<Variable, Dim, T> Variable::constituents() {
  auto &model = requireT<BinArrayModel<T>>(data());
  return {bin_indices(), model.bin_dim(), model.buffer()};
}

template <class T> const T &Variable::bin_buffer() const {
  auto &model = requireT<const BinArrayModel<T>>(data());
  return model.buffer();
}

template <class T> T &Variable::bin_buffer() {
  auto &model = requireT<BinArrayModel<T>>(data());
  return model.buffer();
}

namespace {
auto contiguous_indices(const Variable &parent, const Dimensions &dims) {
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

template <class T> class BinVariableMakerCommon : public AbstractVariableMaker {
public:
  [[nodiscard]] bool is_bins() const override { return true; }
  [[nodiscard]] Variable empty_like(const Variable &prototype,
                                    const std::optional<Dimensions> &shape,
                                    const Variable &sizes) const override {
    if (shape)
      throw except::TypeError(
          "Cannot specify shape in `empty_like` for prototype with bins, shape "
          "must be given by shape of `sizes`.");
    const auto [indices, dim, buf] = prototype.constituents<T>();
    auto sizes_ = sizes;
    if (!sizes.is_valid()) {
      const auto &[begin, end] = unzip(indices);
      sizes_ = end - begin;
    }
    const auto end = cumsum(sizes_);
    const auto begin = end - sizes_;
    const auto size = sum(end - begin).template value<scipp::index>();
    return make_bins(zip(begin, end), dim, resize_default_init(buf, dim, size));
  }
};

template <class T> class BinVariableMaker : public BinVariableMakerCommon<T> {
private:
  const Variable &
  bin_parent(const typename AbstractVariableMaker::parent_list &parents) const {
    constexpr auto is_bins = [](const Variable &x) {
      return x.dtype() == dtype<bucket<T>>;
    };
    const auto count = std::count_if(parents.begin(), parents.end(), is_bins);
    if (count == 0)
      throw except::BinnedDataError("Bin cannot have zero parents");
    if (!std::is_same_v<T, Variable> && (count > 1))
      throw except::BinnedDataError(
          "Binary operations such as '+' with binned data are only supported "
          "with dtype=VariableView, got dtype=" +
          to_string(dtype<bucket<T>>) +
          ". See "
          "https://scipp.github.io/user-guide/binned-data/"
          "computation.html#Event-centric-arithmetic for equivalent operations "
          "for binned (event) data.");
    return *std::find_if(parents.begin(), parents.end(), is_bins);
  }
  virtual Variable call_make_bins(const Variable &parent,
                                  const Variable &indices, const Dim dim,
                                  const DType type, const Dimensions &dims,
                                  const units::Unit &unit,
                                  const bool variances) const = 0;

protected:
  const T &buffer(const Variable &var) const {
    return requireT<const BinArrayModel<T>>(var.data()).buffer();
  }
  T buffer(Variable &var) const {
    return requireT<BinArrayModel<T>>(var.data()).buffer();
  }

public:
  Variable create(const DType elem_dtype, const Dimensions &dims,
                  const units::Unit &unit, const bool variances,
                  const typename AbstractVariableMaker::parent_list &parents)
      const override {
    const Variable &parent = bin_parent(parents);
    const auto &[parentIndices, dim, buffer] = parent.constituents<T>();
    auto [indices, size] = contiguous_indices(parentIndices, dims);
    auto bufferDims = buffer.dims();
    bufferDims.resize(dim, size);
    return call_make_bins(parent, indices, dim, elem_dtype, bufferDims, unit,
                          variances);
  }

  Dim elem_dim(const Variable &var) const override {
    return std::get<1>(var.constituents<T>());
  }
  DType elem_dtype(const Variable &var) const override {
    return std::get<2>(var.constituents<T>()).dtype();
  }
  units::Unit elem_unit(const Variable &var) const override {
    return std::get<2>(var.constituents<T>()).unit();
  }
  void expect_can_set_elem_unit(const Variable &var,
                                const units::Unit &u) const override {
    if (elem_unit(var) != u && var.is_slice())
      throw except::UnitError("Partial view on data of variable cannot be "
                              "used to change the unit.");
  }
  void set_elem_unit(Variable &var, const units::Unit &u) const override {
    std::get<2>(var.constituents<T>()).setUnit(u);
  }
  bool hasVariances(const Variable &var) const override {
    return std::get<2>(var.constituents<T>()).hasVariances();
  }
  core::ElementArrayViewParams
  array_params(const Variable &var) const override {
    const auto &[indices, dim, buffer] = var.constituents<T>();
    auto params = var.array_params();
    return {0, // no offset required in buffer since access via indices
            params.dims(),
            params.strides(),
            {dim, buffer.dims(),
             indices.template values<scipp::index_pair>().data()}};
  }
};

extern template class ElementArrayModel<scipp::index_pair>;

template <class T> BinArrayModel<T> copy(const BinArrayModel<T> &model) {
  return BinArrayModel<T>(model.indices()->clone(), model.bin_dim(),
                          copy(model.buffer()));
}

template <class T>
BinArrayModel<T>::BinArrayModel(const VariableConceptHandle &indices,
                                const Dim dim, T buffer)
    : BinModelBase<Indices>(indices, dim), m_buffer(std::move(buffer)) {}

template <class T> VariableConceptHandle BinArrayModel<T>::clone() const {
  return std::make_shared<BinArrayModel<T>>(variable::copy(*this));
}

template <class T>
bool BinArrayModel<T>::operator==(const BinArrayModel &other) const noexcept {
  if (indices()->dtype() != core::dtype<scipp::index_pair> ||
      other.indices()->dtype() != core::dtype<scipp::index_pair>)
    return false;
  const auto &i1 = requireT<const ElementArrayModel<range_type>>(*indices());
  const auto &i2 =
      requireT<const ElementArrayModel<range_type>>(*other.indices());
  return equals_impl(i1.values(), i2.values()) &&
         this->bin_dim() == other.bin_dim() && m_buffer == other.m_buffer;
}

template <class T>
VariableConceptHandle
BinArrayModel<T>::makeDefaultFromParent(const scipp::index size) const {
  return std::make_shared<BinArrayModel>(
      makeVariable<range_type>(Dims{Dim::X}, Shape{size}).data_handle(),
      this->bin_dim(), T{m_buffer.slice({this->bin_dim(), 0, 0})});
}

template <class T>
VariableConceptHandle
BinArrayModel<T>::makeDefaultFromParent(const Variable &shape) const {
  const auto end = cumsum(shape);
  const auto begin = end - shape;
  const auto size =
      end.dims().volume() > 0 ? end.values<scipp::index>().as_span().back() : 0;
  return std::make_shared<BinArrayModel>(
      zip(begin, begin).data_handle(), this->bin_dim(),
      resize_default_init(m_buffer, this->bin_dim(), size));
}

template <class T> void BinArrayModel<T>::assign(const VariableConcept &other) {
  *this = requireT<const BinArrayModel<T>>(other);
}

template <class T>
ElementArrayView<const typename BinArrayModel<T>::range_type>
BinArrayModel<T>::index_values(const core::ElementArrayViewParams &base) const {
  return requireT<const ElementArrayModel<range_type>>(*this->indices())
      .values(base);
}

template <class T>
Variable make_bins_impl(Variable indices, const Dim dim, T &&buffer) {
  indices.setDataHandle(std::make_unique<variable::BinArrayModel<T>>(
      indices.data_handle(), dim, std::move(buffer)));
  return indices;
}

/// Macro for instantiating classes and functions required for support a new
/// bin dtype in Variable.
#define INSTANTIATE_BIN_ARRAY_VARIABLE(name, ...)                              \
  template <> struct model<core::bin<__VA_ARGS__>> {                           \
    using type = BinArrayModel<__VA_ARGS__>;                                   \
  };                                                                           \
  template SCIPP_EXPORT BinArrayModel<__VA_ARGS__> copy(                       \
      const BinArrayModel<__VA_ARGS__> &);                                     \
  template SCIPP_EXPORT Variable make_bins_impl(Variable, const Dim,           \
                                                __VA_ARGS__ &&);               \
  template class SCIPP_EXPORT BinArrayModel<__VA_ARGS__>;                      \
  INSTANTIATE_VARIABLE_BASE(name, core::bin<__VA_ARGS__>)                      \
  template SCIPP_EXPORT std::tuple<Variable, Dim, __VA_ARGS__>                 \
  Variable::constituents<__VA_ARGS__>() const;                                 \
  template SCIPP_EXPORT const __VA_ARGS__ &Variable::bin_buffer<__VA_ARGS__>() \
      const;                                                                   \
  template SCIPP_EXPORT __VA_ARGS__ &Variable::bin_buffer<__VA_ARGS__>();      \
  template SCIPP_EXPORT std::tuple<Variable, Dim, __VA_ARGS__>                 \
  Variable::constituents<__VA_ARGS__>();                                       \
  template SCIPP_EXPORT std::tuple<Variable, Dim, __VA_ARGS__>                 \
  Variable::to_constituents<__VA_ARGS__>();

} // namespace scipp::variable
