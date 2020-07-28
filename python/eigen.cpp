// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet

#include "numpy.h"
#include "pybind11.h"
#include "scipp/core/string.h"

namespace py = pybind11;

void init_eigen(py::module &m) {
  m.def(
      "rotation_matrix_from_quaternion_coeffs", [](py::array_t<double> value) {
        if (value.size() != 4)
          throw std::runtime_error("Incompatible list size: expected size 4.");
        return Eigen::Quaterniond(value.cast<std::vector<double>>().data())
            .toRotationMatrix();
      });
}
