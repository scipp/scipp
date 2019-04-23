// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef APPLY_H
#define APPLY_H

#include "variable.h"
#include "visit.h"

namespace scipp::core {

/// Apply functor to variables of given arguments.
template <class... Ts, class Op, class Var, class... Vars>
void apply_in_place(Op op, Var &&var, const Vars &... vars) {
  try {
    scipp::core::visit_impl<Ts...>::apply(op, var.dataHandle().variant(),
                                          vars.dataHandle().variant()...);
  } catch (const std::bad_variant_access &) {
    throw except::TypeError("");
  }
}

} // namespace scipp::core

#endif // APPLY_H
