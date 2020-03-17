// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/event.h"
#include "scipp/core/groupby.h"
#include "scipp/core/histogram.h"
#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

namespace scipp::core {

namespace {
template <class T> auto copy_map(const T &map) {
  std::map<typename T::key_type, typename T::mapped_type> out;
  for (const auto &[key, item] : map)
    out.emplace(key, item);
  return out;
}

DataArray
filter_recurse(const DataArrayConstView &unaligned,
               const scipp::span<const std::pair<Dim, Variable>> bounds,
               const AttrPolicy attrPolicy) {
  const auto &[dim, interval] = bounds[0];
  const auto filtered = groupby(unaligned, dim, interval).copy(0, attrPolicy);
  if (bounds.size() == 1)
    return filtered;
  return filter_recurse(filtered, bounds.subspan(1), attrPolicy);
}
} // namespace

/// Return the bounds of all realigned dimensions.
std::vector<std::pair<Dim, Variable>> DataArrayConstView::bounds() const {
  std::vector<std::pair<Dim, Variable>> bounds;
  for (const auto &item : slices()) {
    const auto s = item.first;
    const auto dim = s.dim();
    // Only process realigned dims
    if (unaligned().dims().contains(dim) || !unaligned().coords().contains(dim))
      continue;
    const auto left = s.begin();
    const auto right = s.end() == -1 ? left + 1 : s.end();
    const auto coord = m_dataset->coords()[dim];
    bounds.emplace_back(dim, concatenate(coord.slice({dim, left}),
                                         coord.slice({dim, right}), dim));
  }
  return bounds;
}

namespace {
std::optional<DataArray> optional_unaligned(const DataArrayConstView &view,
                                            const AttrPolicy attrPolicy) {
  if (view.hasData()) {
    return std::nullopt;
  } else {
    const auto bounds = view.bounds();
    return bounds.empty()
               ? copy(view.unaligned(), attrPolicy)
               : filter_recurse(view.unaligned(), bounds, attrPolicy);
  }
}
} // namespace

DataArray::DataArray(const DataArrayConstView &view,
                     const AttrPolicy attrPolicy)
    : DataArray(
          view.hasData() ? Variable(view.data()) : Variable{},
          copy_map(view.coords()), copy_map(view.masks()),
          attrPolicy == AttrPolicy::Keep ? copy_map(view.attrs())
                                         : std::map<std::string, Variable>{},
          view.name(), optional_unaligned(view, attrPolicy), view.dims()) {}

DataArray::operator DataArrayConstView() const { return get(); }
DataArray::operator DataArrayView() { return get(); }

void requireValid(const DataArray &a) {
  if (!a)
    throw std::runtime_error("Invalid DataArray.");
}

DataArrayConstView DataArray::get() const {
  requireValid(*this);
  return *m_holder.begin();
}

DataArrayView DataArray::get() {
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

// This is a temporary helper that will be removed. Once we have proper support
// for unaligned data arrays, direct operations between event data and
// histograms will not be supported any more. Instead, the aligned wrapper of
// the unaligned event data will provide the dimension information.
Dim guess_histogram_dim(const DataArrayConstView &a,
                        const DataArrayConstView &b) {
  Dim hist(Dim::Invalid);
  for (const auto &[dim, coord] : a.coords())
    if (is_events(coord))
      hist = dim;
  for (const auto &[dim, coord] : b.coords())
    if (is_events(coord))
      hist = dim;
  return hist;
}

bool is_sparse_and_histogram(const DataArrayConstView &a,
                             const DataArrayConstView &b, const Dim dim) {
  // TODO Such an indirect check should not be necessary any more. Operations
  // mixing dense and event data are probably only going to be handled using an
  // aligning wrapper, so the sparse dim is known, and coord checks take place
  // on that higher level.
  return dim != Dim::Invalid && (is_histogram(a, dim) || is_histogram(b, dim));
}
} // namespace

template <class Coord, class Edges, class Weights>
auto apply_op_sparse_dense(const Coord &coord, const Edges &edges,
                           const Weights &weights) {
  using W = std::decay_t<decltype(weights)>;
  constexpr bool vars = is_ValueAndVariance_v<W>;
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
  if (scipp::numeric::is_linspace(edges)) {
    const auto [offset, nbin, scale] = linear_edge_params(edges);
    for (const auto c : coord) {
      const auto bin = (c - offset) * scale;
      using w_type = decltype(get(weights, bin));
      constexpr w_type out_of_bounds(0.0);
      w_type w = bin < 0.0 || bin >= nbin ? out_of_bounds : get(weights, bin);
      if constexpr (vars) {
        out_vals.emplace_back(w.value);
        out_vars.emplace_back(w.variance);
      } else {
        out_vals.emplace_back(w);
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
template <class Coord, class Edge, class Weight>
using args =
    std::tuple<sparse_container<Coord>, span<const Edge>, span<const Weight>>;
} // namespace sparse_dense_op_impl_detail

Variable sparse_dense_op_impl(const VariableConstView &sparseCoord_,
                              const VariableConstView &edges_,
                              const VariableConstView &weights_,
                              const Dim dim) {
  using namespace sparse_dense_op_impl_detail;
  return transform<
      std::tuple<args<double, double, double>, args<float, double, double>,
                 args<float, float, float>, args<double, float, float>>>(
      sparseCoord_, subspan_view(edges_, dim), subspan_view(weights_, dim),
      overloaded{[](const auto &... a) { return apply_op_sparse_dense(a...); },
                 transform_flags::expect_no_variance_arg<0>,
                 transform_flags::expect_no_variance_arg<1>,
                 [](const units::Unit &sparse, const units::Unit &edges,
                    const units::Unit &weights) {
                   expect::equals(sparse, edges);
                   return weights;
                 }});
}

DataArray &DataArray::operator+=(const DataArrayConstView &other) {
  expect::coordsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  data() += other.data();
  return *this;
}

DataArray &DataArray::operator-=(const DataArrayConstView &other) {
  expect::coordsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  data() -= other.data();
  return *this;
}

template <class Op>
DataArray &sparse_dense_op_inplace(Op op, DataArray &a,
                                   const DataArrayConstView &b, const Dim dim) {
  if (!is_sparse_and_histogram(a, b, dim)) {
    expect::coordsAreSuperset(a, b);
    union_or_in_place(a.masks(), b.masks());
    op.inplace(a.data(), b.data());
  } else if (is_histogram(b, dim)) {
    // Coord for `dim` in `b` is mismatching that in `a` by definition. Use
    // slice to exclude this from comparison.
    expect::coordsAreSuperset(a, b.slice({dim, 0}));
    union_or_in_place(a.masks(), b.masks());
    if (is_events(a)) {
      // Note the inefficiency here: Always creating temporary sparse data.
      // Could easily avoided, but requires significant code duplication.
      op.inplace(a.data(),
                 sparse_dense_op_impl(a.coords()[dim], b.coords()[dim],
                                      b.data(), dim));
    } else {
      a.setData(
          op(a.data(), sparse_dense_op_impl(a.coords()[dim], b.coords()[dim],
                                            b.data(), dim)));
    }
  } else {
    throw except::SparseDataError("Unsupported combination of sparse and dense "
                                  "data in binary arithmetic operation.");
  }
  return a;
}

DataArray &DataArray::operator*=(const DataArrayConstView &other) {
  const auto dim = guess_histogram_dim(*this, other);
  return sparse_dense_op_inplace(Times{}, *this, other, dim);
}

DataArray &DataArray::operator/=(const DataArrayConstView &other) {
  const auto dim = guess_histogram_dim(*this, other);
  return sparse_dense_op_inplace(Divide{}, *this, other, dim);
}

DataArray &DataArray::operator+=(const VariableConstView &other) {
  data() += other;
  return *this;
}

DataArray &DataArray::operator-=(const VariableConstView &other) {
  data() -= other;
  return *this;
}

DataArray &DataArray::operator*=(const VariableConstView &other) {
  data() *= other;
  return *this;
}

DataArray &DataArray::operator/=(const VariableConstView &other) {
  data() /= other;
  return *this;
}

DataArray operator+(const DataArrayConstView &a, const DataArrayConstView &b) {
  return DataArray(a.data() + b.data(), union_(a.coords(), b.coords()),
                   union_or(a.masks(), b.masks()));
}

DataArray operator-(const DataArrayConstView &a, const DataArrayConstView &b) {
  return {a.data() - b.data(), union_(a.coords(), b.coords()),
          union_or(a.masks(), b.masks())};
}

template <class Op>
auto sparse_dense_op(Op op, const DataArrayConstView &a,
                     const DataArrayConstView &b, const Dim dim) {
  if (!is_sparse_and_histogram(a, b, dim))
    return op(a.data(), b.data());
  if (is_histogram(b, dim)) {
    // not in-place so type promotion can happen
    return op(a.data(), sparse_dense_op_impl(a.coords()[dim], b.coords()[dim],
                                             b.data(), dim));
  }
  // histogram divided by sparse not supported, would typically result in unit
  // 1/counts which is meaningless
  if constexpr (std::is_same_v<Op, Times>)
    return sparse_dense_op(op, b, a, dim);

  throw except::SparseDataError("Unsupported combination of sparse and dense "
                                "data in binary arithmetic operation.");
}

auto sparse_dense_coord_union(const DataArrayConstView &a,
                              const DataArrayConstView &b, const Dim dim) {
  if (!is_sparse_and_histogram(a, b, dim))
    return union_(a.coords(), b.coords());
  // Use slice to remove dense coord, since output will be sparse.
  if (is_histogram(b, dim))
    return union_(a.coords(), b.slice({dim, 0}).coords());
  else
    return union_(a.slice({dim, 0}).coords(), b.coords());
}

DataArray operator*(const DataArrayConstView &a, const DataArrayConstView &b) {
  const auto dim = guess_histogram_dim(a, b);
  const auto data = sparse_dense_op(Times{}, a, b, dim);
  const auto coords = sparse_dense_coord_union(a, b, dim);
  return {std::move(data), std::move(coords), union_or(a.masks(), b.masks())};
}

DataArray operator/(const DataArrayConstView &a, const DataArrayConstView &b) {
  const auto dim = guess_histogram_dim(a, b);
  const auto data = sparse_dense_op(Divide{}, a, b, dim);
  const auto coords = sparse_dense_coord_union(a, b, dim);
  return {std::move(data), std::move(coords), union_or(a.masks(), b.masks())};
}

DataArray operator+(const DataArrayConstView &a, const VariableConstView &b) {
  return DataArray(a.data() + b, a.coords(), a.masks(), a.attrs());
}

DataArray operator-(const DataArrayConstView &a, const VariableConstView &b) {
  return DataArray(a.data() - b, a.coords(), a.masks(), a.attrs());
}

DataArray operator*(const DataArrayConstView &a, const VariableConstView &b) {
  return DataArray(a.data() * b, a.coords(), a.masks(), a.attrs());
}

DataArray operator/(const DataArrayConstView &a, const VariableConstView &b) {
  return DataArray(a.data() / b, a.coords(), a.masks(), a.attrs());
}

DataArray operator+(const VariableConstView &a, const DataArrayConstView &b) {
  return DataArray(a + b.data(), b.coords(), b.masks(), b.attrs());
}

DataArray operator-(const VariableConstView &a, const DataArrayConstView &b) {
  return DataArray(a - b.data(), b.coords(), b.masks(), b.attrs());
}

DataArray operator*(const VariableConstView &a, const DataArrayConstView &b) {
  return DataArray(a * b.data(), b.coords(), b.masks(), b.attrs());
}

DataArray operator/(const VariableConstView &a, const DataArrayConstView &b) {
  return DataArray(a / b.data(), b.coords(), b.masks(), b.attrs());
}

DataArray astype(const DataArrayConstView &var, const DType type) {
  return DataArray(astype(var.data(), type), var.coords(), var.masks(),
                   var.attrs());
}

DataArray reciprocal(const DataArrayConstView &a) {
  return DataArray(reciprocal(a.data()), a.coords(), a.masks(), a.attrs());
}

} // namespace scipp::core
