// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/event.h"
#include "scipp/core/dataset.h"
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

/// Broadcast scalar weights of data arrau containing event data.
Variable broadcast_weights(const DataArrayConstView &events) {
  for (const auto &item : events.coords())
    if (is_events(item.second))
      return broadcast(events.data(), item.second);
  throw except::EventDataError(
      "No coord with event lists found, cannot broadcast weights.");
}

} // namespace event
} // namespace scipp::core
