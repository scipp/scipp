// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "dataset_operations_common.h"
#include "scipp/common/numeric.h"
#include "scipp/core/dataset.h"
#include "scipp/core/event.h"
#include "scipp/core/groupby.h"
#include "scipp/core/histogram.h"
#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"
#include "scipp/core/unaligned.h"

namespace scipp::core {

/// Return the bounds of all sliced realigned dimensions.
std::vector<std::pair<Dim, Variable>> DataArrayConstView::slice_bounds() const {
  std::vector<std::pair<Dim, Variable>> bounds;
  std::map<Dim, std::pair<scipp::index, scipp::index>> combined_slices;
  for (const auto &item : slices()) {
    const auto s = item.first;
    const auto dim = s.dim();
    // Only process realigned dims
    if (unaligned().dims().contains(dim) || !unaligned().coords().contains(dim))
      continue;
    const auto left = s.begin();
    const auto right = s.end() == -1 ? left + 1 : s.end();
    if (combined_slices.count(dim)) {
      combined_slices[dim].second = combined_slices[dim].first + right;
      combined_slices[dim].first += left;
    } else {
      combined_slices[dim] = {left, right};
    }
  }
  for (const auto [dim, interval] : combined_slices) {
    const auto [left, right] = interval;
    const auto coord = m_dataset->coords()[dim];
    bounds.emplace_back(dim, concatenate(coord.slice({dim, left}),
                                         coord.slice({dim, right}), dim));
  }
  // TODO As an optimization we could sort the bounds and put those that slice
  // out the largest fraction first, to ensure that `filter_recurse` slices in a
  // potentially faster way.
  return bounds;
}

namespace {

template <class... DataArgs>
auto makeDataArray(const DataArrayConstView &view, const AttrPolicy attrPolicy,
                   DataArgs &&... dataArgs) {
  return DataArray(std::forward<DataArgs>(dataArgs)..., copy_map(view.coords()),
                   copy_map(view.masks()),
                   attrPolicy == AttrPolicy::Keep
                       ? copy_map(view.attrs())
                       : std::map<std::string, Variable>{},
                   view.name());
}

} // namespace

DataArray::DataArray(const DataArrayConstView &view,
                     const AttrPolicy attrPolicy)
    : DataArray(view.hasData()
                    ? makeDataArray(view, attrPolicy, Variable(view.data()))
                    : makeDataArray(
                          view, attrPolicy,
                          UnalignedData{view.dims(), unaligned::filter_recurse(
                                                         view.unaligned(),
                                                         view.slice_bounds(),
                                                         attrPolicy)})) {}

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

/// Remove any coords added by realign and switch `data` to refer to previous
/// `unaligned` content.
void DataArray::drop_alignment() {
  if (hasData())
    throw except::RealignedDataError(
        "Does not contain unaligned data, cannot drop alignment.");
  auto [dims, array] = std::move(*m_holder.m_data.begin()->second.unaligned);
  const auto move_items = [&dims](auto &from, const auto &to) {
    for (auto &[key, value] : from)
      if (dims.contains(value.dims()))
        to.set(key, std::move(value));
  };
  array.setName(name());
  move_items(m_holder.m_coords, array.coords());
  move_items(m_holder.m_masks, array.masks());
  move_items(m_holder.m_attrs, array.attrs());
  *this = std::move(array);
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

namespace {
bool is_self_append(const DataArrayConstView &a, const DataArrayConstView &b) {
  return &a.underlying() == &b.underlying();
}
} // namespace

DataArray &DataArray::operator+=(const DataArrayConstView &other) {
  expect::coordsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  if (hasData() && other.hasData()) {
    data() += other.data();
  } else {
    if (is_self_append(*this, other))
      event::append(unaligned(), copy(other.unaligned()));
    else
      event::append(unaligned(), other.unaligned());
  }
  return *this;
}

DataArray &DataArray::operator-=(const DataArrayConstView &other) {
  expect::coordsAreSuperset(*this, other);
  union_or_in_place(masks(), other.masks());
  if (hasData() && other.hasData()) {
    data() -= other.data();
  } else {
    const DataArray tmp =
        is_self_append(*this, other) ? copy(other.unaligned()) : DataArray{};
    auto other_unaligned = tmp ? DataArrayConstView(tmp) : other.unaligned();
    unaligned().data() *= -1.0;
    event::append(unaligned(), other_unaligned);
    unaligned().data() *= -1.0;
  }
  return *this;
}

namespace {
void expect_hist(const DataArrayConstView &b, const Dim dim) {
  if (!is_histogram(b, dim))
    throw except::RealignedDataError(
        "Multiplication/division of realigned data requires histogram has "
        "second operand.");
}
} // namespace

template <class Op>
DataArray &sparse_dense_op_inplace(Op op, DataArray &a,
                                   const DataArrayConstView &b) {
  if (unaligned::is_realigned_events(a)) {
    const auto &bins = unaligned::realigned_event_coord(a);
    const Dim dim = bins.dims().inner();
    expect_hist(b, dim);
    expect::coordsAreSuperset(a, b);
    union_or_in_place(a.masks(), b.masks());
    const auto &events = a.unaligned();
    auto weight_scale =
        sparse_dense_op_impl(events.coords()[dim], bins, b.data(), dim);
    if (is_events(events.data())) {
      // Note the inefficiency here: Always creating temporary sparse data.
      // Could easily avoided, but requires significant code duplication.
      op.inplace(events.data(), weight_scale);
    } else {
      events.setData(op(events.data(), weight_scale));
    }
  } else if (!a.hasData()) {
    throw except::RealignedDataError(
        "Operation between realigned data and histograms are currently only "
        "supported for event data.");
  } else {
    expect::coordsAreSuperset(a, b);
    union_or_in_place(a.masks(), b.masks());
    op.inplace(a.data(), b.data());
  }
  return a;
}

DataArray &DataArray::operator*=(const DataArrayConstView &other) {
  return sparse_dense_op_inplace(Times{}, *this, other);
}

DataArray &DataArray::operator/=(const DataArrayConstView &other) {
  return sparse_dense_op_inplace(Divide{}, *this, other);
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
  if (a.hasData() && b.hasData()) {
    return DataArray(a.data() + b.data(), union_(a.coords(), b.coords()),
                     union_or(a.masks(), b.masks()));
  } else {
    DataArray out(a);
    out += b; // No broadcast possible for now
    return out;
  }
}

DataArray operator-(const DataArrayConstView &a, const DataArrayConstView &b) {
  if (a.hasData() && b.hasData()) {
    return {a.data() - b.data(), union_(a.coords(), b.coords()),
            union_or(a.masks(), b.masks())};
  } else {
    DataArray out(a);
    out -= b; // No broadcast possible for now
    return out;
  }
}

template <class Op>
auto sparse_dense_op(Op op, const DataArrayConstView &a,
                     const DataArrayConstView &b) {
  if (unaligned::is_realigned_events(a)) {
    const auto &bins = unaligned::realigned_event_coord(a);
    const Dim dim = bins.dims().inner();
    expect_hist(b, dim);
    const auto &events = a.unaligned();
    auto weight_scale =
        sparse_dense_op_impl(events.coords()[dim], bins, b.data(), dim);

    std::map<Dim, Variable> coords;
    for (const auto &[d, coord] : events.coords()) {
      if (is_events(coord))
        coords.emplace(d, coord);
    }
    // Note that event masks are not supported.
    return UnalignedData{a.dims(),
                         DataArray(op(events.data(), std::move(weight_scale)),
                                   std::move(coords))};
  } else {
    // histogram divided by events not supported, would typically result in unit
    // 1/counts which is meaningless
    if constexpr (std::is_same_v<Op, Times>)
      return sparse_dense_op(op, b, a);

    throw except::RealignedDataError(
        "Unsupported combination of realigned and dense "
        "data in binary arithmetic operation.");
  }
}

namespace {
template <class Op>
DataArray apply_mul_or_div(Op op, const DataArrayConstView &a,
                           const DataArrayConstView &b) {
  if (unaligned::is_realigned_events(a) || unaligned::is_realigned_events(b))
    return {sparse_dense_op(op, a, b), union_(a.coords(), b.coords()),
            union_or(a.masks(), b.masks())};
  else
    return {op(a.data(), b.data()), union_(a.coords(), b.coords()),
            union_or(a.masks(), b.masks())};
}
} // namespace

DataArray operator*(const DataArrayConstView &a, const DataArrayConstView &b) {
  return apply_mul_or_div(Times{}, a, b);
}

DataArray operator/(const DataArrayConstView &a, const DataArrayConstView &b) {
  return apply_mul_or_div(Divide{}, a, b);
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
