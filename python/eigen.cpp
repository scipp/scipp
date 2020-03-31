// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet

#include "numpy.h"
#include "pybind11.h"

namespace py = pybind11;

void init_eigen(py::module &m) {

  py::class_<Eigen::Quaterniond>(m, "Quat", py::buffer_protocol())
      .def(py::init([](py::buffer b) {
        // Request a buffer descriptor from Python
        py::buffer_info info = b.request();

        // Some sanity checks
        if (info.format !=
            py::format_descriptor<Eigen::Quaterniond::Scalar>::format())
          throw std::runtime_error(
              "Incompatible format: expected a double array.");

        if (info.ndim != 1)
          throw std::runtime_error(
              "Incompatible buffer dimension: expected 1 dimension.");

        if (info.size != 4)
          throw std::runtime_error("Incompatible array size: expected size 4.");

        auto map = Eigen::Map<Eigen::Quaterniond>(
            static_cast<Eigen::Quaterniond::Scalar *>(info.ptr));

        return Eigen::Quaterniond(map);
      }))
      .def("x", [](const Eigen::Quaterniond &self) { return self.x(); })
      .def("y", [](const Eigen::Quaterniond &self) { return self.y(); })
      .def("z", [](const Eigen::Quaterniond &self) { return self.z(); })
      .def("w", [](const Eigen::Quaterniond &self) { return self.w(); })
      .def("coeffs",
           [](const Eigen::Quaterniond &self) { return self.coeffs(); })
      .def("to_rotation_matrix",
           [](const Eigen::Quaterniond &self) {
             return self.toRotationMatrix();
           })
      .def_buffer([](Eigen::Quaterniond &q) -> py::buffer_info {
        // See
        // https://pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html#buffer-protocol
        return py::buffer_info(
            q.coeffs().data(), sizeof(Eigen::Quaterniond::Scalar),
            py::format_descriptor<Eigen::Quaterniond::Scalar>::format(), 1, {4},
            {sizeof(Eigen::Quaterniond::Scalar)});
      });
}
