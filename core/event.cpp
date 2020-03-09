// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/event.h"
#include "scipp/core/transform.h"

namespace scipp::core::event {

Variable concatenate(const VariableConstView &a, const VariableConstView &b) {
  Variable out(a);
  transform_in_place<pair_self_t<event_list<double>, event_list<float>,
                                 event_list<int64_t>, event_list<int32_t>>>(
      out, b,
      overloaded{[](auto &a_, const auto &b_) {
                   a_.insert(a_.end(), b_.begin(), b_.end());
                 },
                 [](units::Unit &a_, const units::Unit &b_) {
                   expect::equals(a_, b_);
                 }});
  return out;
}

} // namespace scipp::core::event
