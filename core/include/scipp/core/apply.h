// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_APPLY_H
#define SCIPP_CORE_APPLY_H

#include "scipp/core/except.h"
#include "scipp/core/variable.h"
#include "scipp/core/visit.h"

namespace scipp::core {

/// Apply functor to variables of given arguments.
template <class... Ts, class Op, class Var, class... Vars>
void apply_in_place(Op op, Var &&var, const Vars &... vars) {
  try {
    scipp::core::visit_impl<Ts...>::apply(op, var.dataHandle(),
                                          vars.dataHandle()...);
  } catch (const std::bad_variant_access &) {
    throw except::TypeError("Cannot apply operation to item dtypes: ", var,
                            vars...);
  }
}

} // namespace scipp::core

#endif // SCIPP_CORE_APPLY_H
