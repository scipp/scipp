// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_PYTHON_SHAPED_VIEW_H
#define SCIPP_PYTHON_SHAPED_VIEW_H

#include "scipp/core/variable.h"

#include "pybind11.h"

namespace py = pybind11;
using namespace scipp;
using namespace scipp::core;

namespace scipp::python {

template <class T> struct ShapedView {
  ShapedView(const T &data_, const span<const scipp::index> &shape_)
      : data(data_), shape(shape_.begin(), shape_.end()) {}
  const T data;
  const std::vector<scipp::index> shape;
};

} // namespace scipp::python

#endif // SCIPP_PYTHON_SHAPED_VIEW_H
