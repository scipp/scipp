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

template <class T> using ConstViewRef = const typename T::const_view_type &;

template <class T> using ViewRef = const typename T::view_type &;

template <class T> using View = typename T::view_type;
