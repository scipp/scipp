// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include "scipp/dataset/dataset.h"

using namespace scipp;

struct MoveableVariable {
  Variable var;
};
struct MoveableDataArray {
  dataset::DataArray data;
};
