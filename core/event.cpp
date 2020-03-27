// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/event.h"
#include "scipp/core/dataset.h"
#include "scipp/core/subspan_view.h"
#include "scipp/core/transform.h"

namespace scipp::core {
/// Return true if a variable contains events
bool is_events(const VariableConstView &var) {
  const auto type = var.dtype();
  return type == dtype<sparse_container<double>> ||
         type == dtype<sparse_container<float>> ||
         type == dtype<sparse_container<int64_t>> ||
         type == dtype<sparse_container<int32_t>>;
}

/// Return true if a data array contains events
bool is_events(const DataArrayConstView &array) {
  if (array.hasData() && is_events(array.data()))
    return true;
  for (const auto &item : array.coords())
    if (is_events(item.second))
      return true;
  return false;
}

namespace event {

void append(const VariableView &a, const VariableConstView &b) {
  transform_in_place<pair_self_t<event_list<double>, event_list<float>,
                                 event_list<int64_t>, event_list<int32_t>>>(
      a, b,
      overloaded{[](auto &a_, const auto &b_) {
                   a_.insert(a_.end(), b_.begin(), b_.end());
                 },
                 [](units::Unit &a_, const units::Unit &b_) {
                   expect::equals(a_, b_);
                 }});
}

void append(const DataArrayView &a, const DataArrayConstView &b) {
  if (!is_events(a) || !is_events(b))
    throw except::EventDataError("Cannot concatenate non-event data.");

  if (is_events(a.data())) {
    append(a.data(), is_events(b.data()) ? b.data() : broadcast_weights(b));
  } else if (is_events(b.data())) {
    a.setData(concatenate(broadcast_weights(a), b.data()));
  } else if (a.data() != b.data()) {
    a.setData(concatenate(broadcast_weights(a), broadcast_weights(b)));
  } else {
    // Do nothing for identical scalar weights
  }
  for (const auto &[dim, coord] : a.coords())
    if (is_events(coord))
      append(coord, b.coords()[dim]);
    else
      expect::equals(coord, b.coords()[dim]);
}

Variable concatenate(const VariableConstView &a, const VariableConstView &b) {
  Variable out(a);
  append(out, b);
  return out;
}

DataArray concatenate(const DataArrayConstView &a,
                      const DataArrayConstView &b) {
  DataArray out(a);
  append(out, b);
  return out;
}

/// Broadcast dense variable to same "event shape" as `shape`.
///
/// The return value has the same unit as `dense`, but the dtype is changed to
/// `event_list<input-dtype>` and each event list has the same length as given
/// by the event lists in `shape`.
Variable broadcast(const VariableConstView &dense,
                   const VariableConstView &shape) {
  return dense +
         astype(shape * (0.0 * (dense.unit() / shape.unit())), dense.dtype());
}

/// Broadcast scalar weights of data array containing event data.
Variable broadcast_weights(const DataArrayConstView &events) {
  for (const auto &item : events.coords())
    if (is_events(item.second))
      return broadcast(events.data(), item.second);
  throw except::EventDataError(
      "No coord with event lists found, cannot broadcast weights.");
}

/// Return the sizes of the events lists in var
Variable sizes(const VariableConstView &var) {
  return transform<event_list<double>, event_list<float>>(
      var, overloaded{transform_flags::expect_no_variance_arg<0>,
                      [](const auto &x) { return scipp::size(x); },
                      [](const units::Unit &) {
                        return units::Unit(units::dimensionless);
                      }});
}

/// Resize variable of event lists to sizes given by event list in shape array.
void resize_to(const VariableView &var, const DataArrayConstView &shape) {
  for (const auto &item : shape.coords())
    if (is_events(item.second)) {
      transform_in_place<
          std::tuple<std::tuple<event_list<bool>, scipp::index>>>(
          var, sizes(item.second),
          overloaded{transform_flags::expect_no_variance_arg<1>,
                     [](auto &x, const auto &size) { return x.resize(size); },
                     [](units::Unit &, const units::Unit &) {}});
      return;
    }
  throw except::EventDataError(
      "No event lists found in target shape, cannot resize.");
}

namespace filter_detail {
template <class T>
using make_select_args =
    std::tuple<event_list<bool>, event_list<T>, span<const T>>;
template <class T>
using copy_if_args = std::tuple<event_list<T>, event_list<bool>>;

constexpr auto copy_if = [](const VariableConstView &var,
                            const VariableConstView &select) {
  return transform<std::tuple<copy_if_args<double>, copy_if_args<float>>>(
      var, select,
      overloaded{
          transform_flags::expect_no_variance_arg<1>,
          [](const auto &var_, const auto &select_) {
            using VarT = std::decay_t<decltype(var_)>;
            using Events = event_list<typename VarT::value_type>;
            std::conditional_t<detail::is_ValuesAndVariances_v<VarT>,
                               std::pair<Events, Events>, Events>
                out;
            const auto size = scipp::size(var_);
            for (scipp::index i = 0; i < size; ++i) {
              if (select_[i]) {
                if constexpr (detail::is_ValuesAndVariances_v<VarT>) {
                  out.first.push_back(var_.values[i]);
                  out.second.push_back(var_.variances[i]);
                } else {
                  out.push_back(var_[i]);
                }
              }
            }
            return out;
          },
          [](const units::Unit &var_, const units::Unit &) { return var_; }});
};

} // namespace filter_detail

DataArray filter(const DataArrayConstView &array,
                 const scipp::span<const std::pair<Dim, Variable>> &bounds) {
  using namespace filter_detail;
  Variable select = makeVariable<event_list<bool>>(Dimensions{array.dims()});
  resize_to(select, array);
  select = ~select;
  for (const auto &[dim, interval] : bounds) {
    transform_in_place<
        std::tuple<make_select_args<double>, make_select_args<float>>>(
        select, array.coords()[dim], subspan_view(interval, dim),
        overloaded{
            transform_flags::expect_no_variance_arg<0>,
            transform_flags::expect_no_variance_arg<1>,
            transform_flags::expect_no_variance_arg<2>,
            [](auto &select_, const auto &coord_, const auto &interval_) {
              const auto low = interval_[0];
              const auto high = interval_[1];
              const auto size = scipp::size(coord_);
              for (scipp::index i = 0; i < size; ++i)
                if (coord_[i] < low || coord_[i] >= high)
                  select_[i] = false;
            },
            [](units::Unit &, const units::Unit &coord_,
               const units::Unit &interval_) {
              expect::equals(coord_, interval_);
            }});
  }
  std::map<Dim, Variable> coords;
  for (const auto &[d, coord] : array.coords())
    coords.emplace(d, is_events(coord) ? copy_if(coord, select) : copy(coord));

  return DataArray{is_events(array.data()) ? copy_if(array.data(), select)
                                           : copy(array.data()),
                   std::move(coords), array.masks(), array.attrs()};
}

} // namespace event
} // namespace scipp::core
