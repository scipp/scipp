// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <algorithm>

#include "scipp/common/numeric.h"

#include "scipp/core/histogram.h"

#include "scipp/variable/event.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"
#include "scipp/dataset/groupby.h"
#include "scipp/dataset/histogram.h"
#include "scipp/dataset/unaligned.h"

#include "dataset_operations_common.h"

namespace scipp::dataset {

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
  for (const auto &[dim, interval] : combined_slices) {
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
  constexpr auto move_items = [](auto &from, const auto &to,
                                 const Dimensions &d) {
    for (auto &[key, value] : from)
      if (d.contains(value.dims()))
        to.set(key, std::move(value));
  };
  array.setName(name());
  move_items(m_holder.m_coords, array.coords(), dims);
  move_items(m_holder.m_masks, array.masks(), dims);
  move_items(m_holder.m_attrs, array.attrs(), dims);
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
auto apply_op_events_dense(const Coord &coord, const Edges &edges,
                           const Weights &weights) {
  using W = std::decay_t<decltype(weights)>;
  constexpr bool vars = is_ValueAndVariance_v<W>;
  using ElemT = typename core::detail::element_type_t<W>::value_type;
  using T = event_list<ElemT>;
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
    const auto [offset, nbin, scale] = core::linear_edge_params(edges);
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
    core::expect::histogram::sorted_edges(edges);
    throw std::runtime_error("Non-constant bin width not supported yet.");
  }
  if constexpr (vars)
    return std::pair(std::move(out_vals), std::move(out_vars));
  else
    return out_vals;
}

namespace events_dense_op_impl_detail {
template <class Coord, class Edge, class Weight>
using args =
    std::tuple<event_list<Coord>, span<const Edge>, span<const Weight>>;
} // namespace events_dense_op_impl_detail

Variable events_dense_op_impl(const VariableConstView &eventsCoord_,
                              const VariableConstView &edges_,
                              const VariableConstView &weights_,
                              const Dim dim) {
  using namespace events_dense_op_impl_detail;
  return variable::transform<
      std::tuple<args<double, double, double>, args<float, double, double>,
                 args<float, float, float>, args<double, float, float>>>(
      eventsCoord_, subspan_view(edges_, dim), subspan_view(weights_, dim),
      overloaded{[](const auto &... a) { return apply_op_events_dense(a...); },
                 core::transform_flags::expect_no_variance_arg<0>,
                 core::transform_flags::expect_no_variance_arg<1>,
                 [](const units::Unit &events, const units::Unit &edges,
                    const units::Unit &weights) {
                   core::expect::equals(events, edges);
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
    unaligned().data() *= -1.0 * units::one;
    event::append(unaligned(), other_unaligned);
    unaligned().data() *= -1.0 * units::one;
  }
  return *this;
}

namespace {
Dim realigned_event_histogram_op_dim(const DataArrayConstView &realigned,
                                     const DataArrayConstView &histogram) {
  const auto event_dims = unaligned::realigned_event_dims(realigned);
  const auto edge_dims = edge_dimensions(histogram);
  std::set<Dim> intersect;
  std::set_intersection(event_dims.begin(), event_dims.end(), edge_dims.begin(),
                        edge_dims.end(),
                        std::inserter(intersect, intersect.begin()));
  if (intersect.size() != 1)
    throw except::BinEdgeError(
        "Operation between realigned event data and histogram currently only "
        "supports one histogrammed dimension.");
  return *intersect.begin();
}
} // namespace

template <class Op>
DataArray &events_dense_op_inplace(Op op, DataArray &a,
                                   const DataArrayConstView &b) {
  if (unaligned::is_realigned_events(a)) {
    expect::coordsAreSuperset(a, b);
    union_or_in_place(a.masks(), b.masks());
    const auto &events = a.unaligned();
    const Dim dim = realigned_event_histogram_op_dim(a, b);
    auto weight_scale = event::map(b, events.coords()[dim], dim);
    if (contains_events(events.data())) {
      // Note the inefficiency here: Always creating temporary event data.
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
  return events_dense_op_inplace(Times{}, *this, other);
}

DataArray &DataArray::operator/=(const DataArrayConstView &other) {
  return events_dense_op_inplace(Divide{}, *this, other);
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
                     union_or(a.masks(), b.masks()),
                     intersection(a.attrs(), b.attrs()));
  } else {
    DataArray out(a);
    out += b; // No broadcast possible for now
    return out;
  }
}

DataArray operator-(const DataArrayConstView &a, const DataArrayConstView &b) {
  if (a.hasData() && b.hasData()) {
    return {a.data() - b.data(), union_(a.coords(), b.coords()),
            union_or(a.masks(), b.masks()), intersection(a.attrs(), b.attrs())};
  } else {
    DataArray out(a);
    out -= b; // No broadcast possible for now
    return out;
  }
}

template <class Op>
auto events_dense_op(Op op, const DataArrayConstView &a,
                     const DataArrayConstView &b) {
  if (unaligned::is_realigned_events(a)) {
    const auto &events = a.unaligned();
    const Dim dim = realigned_event_histogram_op_dim(a, b);
    auto weight_scale = event::map(b, events.coords()[dim], dim);
    std::map<Dim, Variable> coords;
    for (const auto &[d, coord] : events.coords()) {
      if (contains_events(coord))
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
      return events_dense_op(op, b, a);

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
    return {events_dense_op(op, a, b), union_(a.coords(), b.coords()),
            union_or(a.masks(), b.masks()), intersection(a.attrs(), b.attrs())};
  else
    return {op(a.data(), b.data()), union_(a.coords(), b.coords()),
            union_or(a.masks(), b.masks()), intersection(a.attrs(), b.attrs())};
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

} // namespace scipp::dataset
