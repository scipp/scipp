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

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/event.h"
#include "scipp/dataset/histogram.h"

namespace scipp::dataset {
/// Return true if a data array contains events
bool contains_events(const DataArrayConstView &array) {
  if (array.hasData() && contains_events(array.data()))
    return true;
  for (const auto &item : array.coords())
    if (contains_events(item.second))
      return true;
  return false;
}

namespace event {

void append(const DataArrayView &a, const DataArrayConstView &b) {
  if (!contains_events(a) || !contains_events(b))
    throw except::EventDataError("Cannot concatenate non-event data.");

  if (contains_events(a.data())) {
    variable::event::append(
        a.data(), contains_events(b.data()) ? b.data() : broadcast_weights(b));
  } else if (contains_events(b.data())) {
    a.setData(variable::event::concatenate(broadcast_weights(a), b.data()));
  } else if (a.data() != b.data()) {
    a.setData(variable::event::concatenate(broadcast_weights(a),
                                           broadcast_weights(b)));
  } else {
    // Do nothing for identical scalar weights
  }
  for (const auto &[dim, coord] : a.coords())
    if (contains_events(coord))
      variable::event::append(coord, b.coords()[dim]);
    else
      core::expect::equals(coord, b.coords()[dim]);
}

DataArray concatenate(const DataArrayConstView &a,
                      const DataArrayConstView &b) {
  DataArray out(a);
  append(out, b);
  return out;
}

/// Broadcast scalar weights of data array containing event data.
Variable broadcast_weights(const DataArrayConstView &events) {
  for (const auto &item : events.coords())
    if (contains_events(item.second))
      return variable::event::broadcast(events.data(), item.second);
  throw except::EventDataError(
      "No coord with event lists found, cannot broadcast weights.");
}

namespace {
/// Return new variable with values copied from `var` if index is included in
/// `select`.
constexpr auto copy_if = [](const VariableConstView &var,
                            const VariableConstView &select) {
  return transform(var, select, core::element::event::copy_if);
};

/// Return list of indices with coord values for given dim inside interval.
template <class T>
const auto make_select = [](const DataArrayConstView &array, const Dim dim,
                            const VariableConstView &interval) {
  return transform(array.coords()[dim], subspan_view(interval, dim),
                   core::element::event::make_select<T>);
};

} // namespace
/// Return filtered event data based on excluding all events with coord values
/// for given dim outside interval.
DataArray filter(const DataArrayConstView &array, const Dim dim,
                 const VariableConstView &interval,
                 const AttrPolicy attrPolicy) {
  const auto &max_event_list_length =
      max(variable::event::sizes(array.coords()[dim]));
  const bool need_64bit_indices =
      max_event_list_length.values<scipp::index>()[0] >
      std::numeric_limits<int32_t>::max();
  const auto select = need_64bit_indices
                          ? make_select<int64_t>(array, dim, interval)
                          : make_select<int32_t>(array, dim, interval);

  std::map<Dim, Variable> coords;
  for (const auto &[d, coord] : array.coords())
    coords.emplace(d, contains_events(coord) ? copy_if(coord, select)
                                             : copy(coord));

  Dataset empty;
  return DataArray{contains_events(array.data()) ? copy_if(array.data(), select)
                                                 : copy(array.data()),
                   std::move(coords), array.masks(),
                   attrPolicy == AttrPolicy::Keep ? array.attrs()
                                                  : empty.attrs(),
                   array.name()};
}

Variable map(const DataArrayConstView &function, const VariableConstView &x,
             Dim dim) {
  if (dim == Dim::Invalid)
    dim = edge_dimension(function);
  return transform(x, subspan_view(function.coords()[dim], dim),
                   subspan_view(function.data(), dim),
                   core::element::event::map);
}

} // namespace event
} // namespace scipp::dataset
