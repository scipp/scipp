// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#ifndef SCIPP_BIND_DETAIL_H
#define SCIPP_BIND_DETAIL_H

#include "scipp/core/dataset.h"

using namespace scipp::core;

struct MoveableVariable {
  Variable var;
};
struct MoveableDataArray {
  DataArray data;
};

#endif // SCIPP_BIND_DETAIL_H
