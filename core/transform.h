// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "variable.h"

namespace scipp::core {

template <class... Ts, class Var, class Op>
void transform_in_place(Var &var, Op op) {
  var.dataHandle().template transform_in_place<Ts...>(op);
}

template <class... TypePairs, class Var1, class Var, class Op>
void transform_in_place(const Var1 &other, Var &&var, Op op) {
  var.dataHandle().template transform_in_place<TypePairs...>(
      op, other.dataHandle());
}

template <class... Ts, class Var, class Op>
Variable transform(const Var &var, Op op) {
  return Variable(var, var.dataHandle().template transform<Ts...>(op));
}

} // namespace scipp::core

#endif // TRANSFORM_H
