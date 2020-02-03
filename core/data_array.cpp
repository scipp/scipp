// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/histogram.h"
#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

namespace scipp::core {

template <class T> auto copy_map(const T &map) {
  std::map<typename T::key_type, typename T::mapped_type> out;
  for (const auto &[key, item] : map)
    out.emplace(key, item);
  return out;
}

DataArray::DataArray(const DataConstProxy &proxy)
    : DataArray(proxy.hasData() ? std::optional<Variable>(proxy.data())
                                : std::optional<Variable>(),
                copy_map(proxy.coords()), copy_map(proxy.labels()),
                copy_map(proxy.masks()), copy_map(proxy.attrs()),
                proxy.name()) {}

DataArray::operator DataConstProxy() const { return get(); }
DataArray::operator DataProxy() { return get(); }

void requireValid(const DataArray &a) {
  if (!a)
    throw std::runtime_error("Invalid DataArray.");
}

DataConstProxy DataArray::get() const {
  requireValid(*this);
  return *m_holder.begin();
}

DataProxy DataArray::get() {
  requireValid(*this);
  return *m_holder.begin();
}

namespace {
struct Times {
  template <class A, class B>
  constexpr auto operator()(const A &a, const B &b) const {
    return a * b;
  }
  template <class A, class B>
  constexpr void inplace(const A &a, const B &b) const {
    a *= b;
  }
};
struct Divide {
  template <class A, class B>
  constexpr auto operator()(const A &a, const B &b) const {
    return a / b;
  }
  template <class A, class B>
  constexpr void inplace(const A &a, const B &b) const {
    a /= b;
  }
};
} // namespace

bool is_sparse_and_histogram(const DataConstProxy &a, const DataConstProxy &b) {
  return (a.dims().sparse() && is_histogram(b, a.dims().sparseDim())) ||
         (b.dims().sparse() && is_histogram(a, b.dims().sparseDim()));
}

template <class Op, class Coord, class Data, class Edges, class Weights>
auto apply_op_sparse_dense(Op op, const Coord &coord, const Data &data,
                           const Edges &edges, const Weights &weights) {
  using D = std::decay_t<decltype(data)>;
  using W = std::decay_t<decltype(weights)>;
  constexpr bool vars = is_ValueAndVariance_v<D> || is_ValueAndVariance_v<W>;
  using ElemT = typename core::detail::element_type_t<W>::value_type;
  using T = sparse_container<ElemT>;
  T out_vals;
  T out_vars;
  out_vals.reserve(coord.size());
  if constexpr (vars)
    out_vars.reserve(coord.size());
  constexpr auto get = [](const auto &x, const scipp::index i) {
    if constexpr (is_ValueAndVariance_v<std::decay_t<decltype(x)>>)
      return ValueAndVariance{x.value[i], x.variance[i]};
    else
      return x[i];
  };
  const auto d = get(data, 0);
  if (scipp::numeric::is_linspace(edges)) {
    const auto [offset, nbin, scale] = linear_edge_params(edges);
    for (const auto c : coord) {
      const auto bin = (c - offset) * scale;
      auto w = get(weights, bin);
      if (bin < 0.0 || bin >= nbin)
        w = 0.0;
      if constexpr (vars) {
        const auto [val, var] = op(d, w);
        out_vals.emplace_back(val);
        out_vars.emplace_back(var);
      } else {
        out_vals.emplace_back(op(d, w));
      }
    }
  } else {
    expect::histogram::sorted_edges(edges);
    throw std::runtime_error("Non-constant bin width not supported yet.");
  }
  if constexpr (vars)
    return std::pair(std::move(out_vals), std::move(out_vars));
  else
    return out_vals;
}

namespace sparse_dense_op_impl_detail {
// Note that currently we require Data = Weight.
template <class Coord, class Data, class Edge, class Weight>
using args = std::tuple<sparse_container<Coord>, span<const Data>,
                        span<const Edge>, span<const Weight>>;
} // namespace sparse_dense_op_impl_detail

template <int ImplicitData, class Op>
Variable sparse_dense_op_impl(Op op, const VariableConstProxy &sparseCoord_,
                              const VariableConstProxy &edges_,
                              const VariableConstProxy &weights_) {
  using namespace sparse_dense_op_impl_detail;
  const Dim dim = sparseCoord_.dims().sparseDim();
  // Sparse data without values has an implicit value of 1 count. If
  // `ImplicitData` is 0 we simply use this function to generate intermediate
  // sparse data that can be multiplied with the existing data by the caller.
  const Variable implicit_data =
      ImplicitData
          ? Variable(weights_.dtype(), Dims{dim}, Shape{1}, Values{1.0},
                     Variances{1.0}, units::Unit(units::counts))
          : Variable(weights_.dtype(), Dims{dim}, Shape{1}, Values{1.0});
  return transform<std::tuple<
      args<double, double, double, double>, args<float, double, double, double>,
      args<float, float, float, float>, args<double, float, float, float>>>(
      sparseCoord_, subspan_view(implicit_data, dim), subspan_view(edges_, dim),
      subspan_view(weights_, dim),
      overloaded{
          [op](const auto &... a) { return apply_op_sparse_dense(op, a...); },
          transform_flags::expect_no_variance_arg<0>,
          transform_flags::expect_no_variance_arg<2>,
          [op](const units::Unit &sparse, const units::Unit &data,
               const units::Unit &edges, const units::Unit &weights) {
            expect::equals(sparse, edges);
            return op(data, weights);
          }});
}

DataArray &DataArray::operator+=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  data() += other.data();
  return *this;
}

DataArray &DataArray::operator-=(const DataConstProxy &other) {
  expect::coordsAndLabelsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  data() -= other.data();
  return *this;
}

template <class Op>
DataArray &sparse_dense_op_inplace(Op op, DataArray &a,
                                   const DataConstProxy &b) {
  if (!is_sparse_and_histogram(a, b)) {
    expect::coordsAndLabelsAreSuperset(a, b);
    union_or_in_place(a.masks(), b.masks());
    op.inplace(a.data(), b.data());
  } else if (a.dims().sparse()) {
    const Dim dim = a.dims().sparseDim();
    // Coord for `dim` in `b` is mismatching that in `a` by definition. Use
    // slice to exclude this from comparison.
    expect::coordsAndLabelsAreSuperset(a, b.slice({dim, 0}));
    union_or_in_place(a.masks(), b.masks());
    if (a.hasData()) {
      // Note the inefficiency here: Always creating temporary sparse data.
      // Could easily avoided, but requires significant code duplication.
      a.data() *= sparse_dense_op_impl<0>(op, a.coords()[dim], b.coords()[dim],
                                          b.data());
    } else {
      a.setData(sparse_dense_op_impl<1>(op, a.coords()[dim], b.coords()[dim],
                                        b.data()));
    }
  } else {
    throw except::SparseDataError("Unsupported combination of sparse and dense "
                                  "data in binary arithmetic operation.");
  }
  return a;
}

DataArray &DataArray::operator*=(const DataConstProxy &other) {
  return sparse_dense_op_inplace(Times{}, *this, other);
}

DataArray &DataArray::operator/=(const DataConstProxy &other) {
  return sparse_dense_op_inplace(Divide{}, *this, other);
}

DataArray &DataArray::operator+=(const VariableConstProxy &other) {
  data() += other;
  return *this;
}

DataArray &DataArray::operator-=(const VariableConstProxy &other) {
  data() -= other;
  return *this;
}

DataArray &DataArray::operator*=(const VariableConstProxy &other) {
  data() *= other;
  return *this;
}

DataArray &DataArray::operator/=(const VariableConstProxy &other) {
  data() /= other;
  return *this;
}

DataArray operator+(const DataConstProxy &a, const DataConstProxy &b) {
  return DataArray(a.data() + b.data(), union_(a.coords(), b.coords()),
                   union_(a.labels(), b.labels()),
                   union_or(a.masks(), b.masks()));
}

DataArray operator-(const DataConstProxy &a, const DataConstProxy &b) {
  return {a.data() - b.data(), union_(a.coords(), b.coords()),
          union_(a.labels(), b.labels()), union_or(a.masks(), b.masks())};
}

template <class Op>
auto sparse_dense_op(Op op, const DataConstProxy &a, const DataConstProxy &b) {
  if (!is_sparse_and_histogram(a, b))
    return op(a.data(), b.data());
  if (a.dims().sparse()) {
    const Dim dim = a.dims().sparseDim();
    if (a.hasData()) {
      // not in-place so type promotion can happen
      return sparse_dense_op_impl<0>(op, a.coords()[dim], b.coords()[dim],
                                     b.data()) *
             a.data();
    } else {
      return sparse_dense_op_impl<1>(op, a.coords()[dim], b.coords()[dim],
                                     b.data());
    }
  }
  // histogram divided by sparse not supported, would typically result in unit
  // 1/counts which is meaningless
  if constexpr (std::is_same_v<Op, Times>)
    return sparse_dense_op(op, b, a);

  throw except::SparseDataError("Unsupported combination of sparse and dense "
                                "data in binary arithmetic operation.");
}

auto sparse_dense_coord_union(const DataConstProxy &a,
                              const DataConstProxy &b) {
  if (!is_sparse_and_histogram(a, b))
    return union_(a.coords(), b.coords());
  // Use slice to remove dense coord, since output will be sparse.
  if (a.dims().sparse())
    return union_(a.coords(), b.slice({a.dims().sparseDim(), 0}).coords());
  else
    return union_(a.slice({b.dims().sparseDim(), 0}).coords(), b.coords());
}

DataArray operator*(const DataConstProxy &a, const DataConstProxy &b) {
  const auto data = sparse_dense_op(Times{}, a, b);
  const auto coords = sparse_dense_coord_union(a, b);
  return {std::move(data), std::move(coords), union_(a.labels(), b.labels()),
          union_or(a.masks(), b.masks())};
}

DataArray operator/(const DataConstProxy &a, const DataConstProxy &b) {
  const auto data = sparse_dense_op(Divide{}, a, b);
  const auto coords = sparse_dense_coord_union(a, b);
  return {std::move(data), std::move(coords), union_(a.labels(), b.labels()),
          union_or(a.masks(), b.masks())};
}

DataArray operator+(const DataConstProxy &a, const VariableConstProxy &b) {
  return DataArray(a.data() + b, a.coords(), a.labels(), a.masks(), a.attrs());
}

DataArray operator-(const DataConstProxy &a, const VariableConstProxy &b) {
  return DataArray(a.data() - b, a.coords(), a.labels(), a.masks(), a.attrs());
}

DataArray operator*(const DataConstProxy &a, const VariableConstProxy &b) {
  return DataArray(a.data() * b, a.coords(), a.labels(), a.masks(), a.attrs());
}

DataArray operator/(const DataConstProxy &a, const VariableConstProxy &b) {
  return DataArray(a.data() / b, a.coords(), a.labels(), a.masks(), a.attrs());
}

DataArray operator+(const VariableConstProxy &a, const DataConstProxy &b) {
  return DataArray(a + b.data(), b.coords(), b.labels(), b.masks(), b.attrs());
}

DataArray operator-(const VariableConstProxy &a, const DataConstProxy &b) {
  return DataArray(a - b.data(), b.coords(), b.labels(), b.masks(), b.attrs());
}

DataArray operator*(const VariableConstProxy &a, const DataConstProxy &b) {
  return DataArray(a * b.data(), b.coords(), b.labels(), b.masks(), b.attrs());
}

DataArray operator/(const VariableConstProxy &a, const DataConstProxy &b) {
  return DataArray(a / b.data(), b.coords(), b.labels(), b.masks(), b.attrs());
}

DataArray astype(const DataConstProxy &var, const DType type) {
  return DataArray(astype(var.data(), type), var.coords(), var.labels(),
                   var.masks(), var.attrs());
}

DataArray reciprocal(const DataConstProxy &a) {
  return DataArray(reciprocal(a.data()), a.coords(), a.labels(), a.masks(),
                   a.attrs());
}

} // namespace scipp::core
