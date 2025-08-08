// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/core/tag_util.h"
#include "scipp/variable/creation.h"

#include "dim.h"
#include "dtype.h"
#include "pybind11.h"
#include "unit.h"

using namespace scipp;

namespace py = pybind11;

template <class T> struct MakeZeros {
  static variable::Variable apply(const std::vector<std::string> &dims,
                                  const std::vector<scipp::index> &shape,
                                  const sc_units::Unit &unit,
                                  const bool with_variances) {
    return with_variances
               ? makeVariable<T>(make_dims(dims, shape), unit, Values{},
                                 Variances{})
               : makeVariable<T>(make_dims(dims, shape), unit, Values{});
  }
};

void init_creation(py::module &m) {
  m.def(
      "empty",
      [](const std::vector<std::string> &dims,
         const std::vector<scipp::index> &shape, const ProtoUnit &unit,
         const py::object &dtype, const bool with_variances,
         const bool aligned) {
        const auto dtype_ = scipp_dtype(dtype);
        py::gil_scoped_release release;
        const auto unit_ = unit_or_default(unit, dtype_);
        return variable::empty(make_dims(dims, shape), unit_, dtype_,
                               with_variances, aligned);
      },
      py::arg("dims"), py::arg("shape"), py::arg("unit") = DefaultUnit{},
      py::arg("dtype") = py::none(), py::arg("with_variances") = false,
      py::arg("aligned") = true);
  m.def(
      "zeros",
      [](const std::vector<std::string> &dims,
         const std::vector<scipp::index> &shape, const ProtoUnit &unit,
         const py::object &dtype, const bool with_variances) {
        const auto dtype_ = scipp_dtype(dtype);
        py::gil_scoped_release release;
        const auto unit_ = unit_or_default(unit, dtype_);
        return core::CallDType<
            double, float, int64_t, int32_t, bool, scipp::core::time_point,
            std::string, Eigen::Vector3d,
            Eigen::Matrix3d>::apply<MakeZeros>(dtype_, dims, shape, unit_,
                                               with_variances);
      },
      py::arg("dims"), py::arg("shape"), py::arg("unit") = DefaultUnit{},
      py::arg("dtype") = py::none(), py::arg("with_variances") = std::nullopt);
  m.def(
      "ones",
      [](const std::vector<std::string> &dims,
         const std::vector<scipp::index> &shape, const ProtoUnit &unit,
         const py::object &dtype, const bool with_variances) {
        const auto dtype_ = scipp_dtype(dtype);
        py::gil_scoped_release release;
        const auto unit_ = unit_or_default(unit, dtype_);
        return variable::ones(make_dims(dims, shape), unit_, dtype_,
                              with_variances);
      },
      py::arg("dims"), py::arg("shape"), py::arg("unit") = DefaultUnit{},
      py::arg("dtype") = py::none(), py::arg("with_variances") = std::nullopt);
}
