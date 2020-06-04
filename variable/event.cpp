// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <limits>

#include "scipp/core/element/event_operations.h"

#include "scipp/variable/event.h"
#include "scipp/variable/operations.h"
#include "scipp/variable/subspan_view.h"
#include "scipp/variable/transform.h"

namespace scipp::variable {
/// Return true if a variable contains events
bool contains_events(const VariableConstView &var) {
  const auto type = var.dtype();
  return type == dtype<event_list<double>> ||
         type == dtype<event_list<float>> ||
         type == dtype<event_list<int64_t>> ||
         type == dtype<event_list<int32_t>>;
}

namespace event {

void append(const VariableView &a, const VariableConstView &b) {
  transform_in_place<
      core::pair_self_t<event_list<double>, event_list<float>,
                        event_list<int64_t>, event_list<int32_t>>>(
      a, b,
      overloaded{[](auto &a_, const auto &b_) {
                   a_.insert(a_.end(), b_.begin(), b_.end());
                 },
                 [](units::Unit &a_, const units::Unit &b_) {
                   core::expect::equals(a_, b_);
                 }});
}

Variable concatenate(const VariableConstView &a, const VariableConstView &b) {
  Variable out(a);
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

/// Return the sizes of the events lists in `events`
Variable sizes(const VariableConstView &events) {
  // To simplify this we would like to use `transform`, but this is currently
  // not possible since the current implementation expects outputs with
  // variances if any of the inputs has variances.
  auto sizes = makeVariable<scipp::index>(events.dims());
  accumulate_in_place<
      core::pair_custom_t<std::tuple<scipp::index, event_list<double>>>,
      core::pair_custom_t<std::tuple<scipp::index, event_list<float>>>,
      core::pair_custom_t<std::tuple<scipp::index, event_list<int64_t>>>,
      core::pair_custom_t<std::tuple<scipp::index, event_list<int32_t>>>>(
      sizes, events,
      overloaded{[](scipp::index &c, const auto &list) { c = list.size(); },
                 core::transform_flags::expect_no_variance_arg<0>});
  return sizes;
}

/// Reserve memory in all event lists in `events`, based on `capacity`.
///
/// To avoid pessimizing reserves, this does nothing if the new capacity is less
/// than the typical logarithmic growth. This yields a 5x speedup in some cases,
/// without apparent negative effect on the other cases.
void reserve(const VariableView &events, const VariableConstView &capacity) {
  transform_in_place<
      core::pair_custom_t<std::tuple<event_list<double>, scipp::index>>,
      core::pair_custom_t<std::tuple<event_list<float>, scipp::index>>,
      core::pair_custom_t<std::tuple<event_list<int64_t>, scipp::index>>,
      core::pair_custom_t<std::tuple<event_list<int32_t>, scipp::index>>>(
      events, capacity,
      overloaded{[](auto &&events_, const scipp::index capacity_) {
                   if (capacity_ > 2 * scipp::size(events_))
                     return events_.reserve(capacity_);
                 },
                 core::transform_flags::expect_no_variance_arg<1>,
                 [](const units::Unit &, const units::Unit &) {}});
}

} // namespace event
} // namespace scipp::variable
