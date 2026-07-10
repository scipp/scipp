// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/eigen.h"
#include "scipp/core/tag_util.h"
#include "scipp/variable/creation.h"

#include "dim.h"
#include "dtype.h"
#include "nanobind.h"
#include "unit.h"

using namespace scipp;

namespace nb = nanobind;

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

void init_creation(nb::module_ &m) {
  m.def(
      "empty",
      [](const std::vector<std::string> &dims,
         const std::vector<scipp::index> &shape, const ProtoUnit &unit,
         const nb::object &dtype, const bool with_variances,
         const bool aligned) {
        const auto dtype_ = scipp_dtype(dtype);
        nb::gil_scoped_release release;
        const auto unit_ = unit_or_default(unit, dtype_);
        return variable::empty(make_dims(dims, shape), unit_, dtype_,
                               with_variances, aligned);
      },
      nb::arg("dims"), nb::arg("shape"), nb::arg("unit") = DefaultUnit{},
      nb::arg("dtype") = nb::none(), nb::arg("with_variances") = false,
      nb::arg("aligned") = true);
  m.def(
      "zeros",
      [](const std::vector<std::string> &dims,
         const std::vector<scipp::index> &shape, const ProtoUnit &unit,
         const nb::object &dtype, const bool with_variances) {
        const auto dtype_ = scipp_dtype(dtype);
        nb::gil_scoped_release release;
        const auto unit_ = unit_or_default(unit, dtype_);
        return core::CallDType<
            double, float, int64_t, int32_t, bool, scipp::core::time_point,
            std::string, Eigen::Vector3d,
            Eigen::Matrix3d>::apply<MakeZeros>(dtype_, dims, shape, unit_,
                                               with_variances);
      },
      nb::arg("dims"), nb::arg("shape"), nb::arg("unit") = DefaultUnit{},
      nb::arg("dtype") = nb::none(), nb::arg("with_variances") = std::nullopt);
  m.def(
      "ones",
      [](const std::vector<std::string> &dims,
         const std::vector<scipp::index> &shape, const ProtoUnit &unit,
         const nb::object &dtype, const bool with_variances) {
        const auto dtype_ = scipp_dtype(dtype);
        nb::gil_scoped_release release;
        const auto unit_ = unit_or_default(unit, dtype_);
        return variable::ones(make_dims(dims, shape), unit_, dtype_,
                              with_variances);
      },
      nb::arg("dims"), nb::arg("shape"), nb::arg("unit") = DefaultUnit{},
      nb::arg("dtype") = nb::none(), nb::arg("with_variances") = std::nullopt);
}
