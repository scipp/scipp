// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet

#include "numpy.h"
#include "pybind11.h"
#include "scipp/core/string.h"

namespace py = pybind11;

void init_eigen(py::module &m) {

  py::class_<Eigen::Quaterniond>(m, "Quat", py::buffer_protocol())
      // Note that when constructing a Quat from a buffer array, the order of
      // the input coefficients is [x, y, z, w], as returned by the `coeffs()`
      // method.
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

        return Eigen::Quaterniond(
            static_cast<Eigen::Quaterniond::Scalar *>(info.ptr));
      }))
      .def(py::init([](py::list value) {
        if (value.size() != 4)
          throw std::runtime_error("Incompatible list size: expected size 4.");

        return Eigen::Quaterniond(value.cast<std::vector<double>>().data());
      }))
      .def("__eq__",
           [](Eigen::Quaterniond &self, Eigen::Quaterniond &other) {
             return self.coeffs() == other.coeffs();
           },
           py::is_operator(), py::call_guard<py::gil_scoped_release>())
      .def("__repr__",
           [](const Eigen::Quaterniond &self) {
             return scipp::core::element_to_string(self);
           })
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
        return py::buffer_info(
            q.coeffs().data(), sizeof(Eigen::Quaterniond::Scalar),
            py::format_descriptor<Eigen::Quaterniond::Scalar>::format(), 1, {4},
            {sizeof(Eigen::Quaterniond::Scalar)});
      });
}
