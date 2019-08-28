// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Igor Gudich

#include "scipp/core/dtype.h"

bool scipp::core::isInt(DType tp) {
  return tp == DType::Int32 || tp == DType::Int64;
}

bool scipp::core::isFloatingPoint(DType tp) {
  return tp == DType::Float || tp == DType::Double ||
         tp == DType::EigenVector3d;
}

bool scipp::core::isBool(DType tp) { return tp == DType::Bool; }
