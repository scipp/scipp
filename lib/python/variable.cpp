// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/units/unit.h"

#include "scipp/common/numeric.h"

#include "scipp/core/dtype.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/core/time_point.h"

#include "scipp/variable/operations.h"
#include "scipp/variable/structures.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/util.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "dim.h"
#include "numpy.h"
#include "pybind11.h"
#include "rename.h"
#include "unit.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T, class Elem, int... N>
void bind_structured_creation(py::module &m, const std::string &name) {
  m.def(
      name.c_str(),
      [](const std::vector<std::string> &labels, py::array_t<Elem> &values,
         const ProtoUnit &unit) {
        if (scipp::size(labels) != values.ndim() - scipp::index(sizeof...(N)))
          throw std::runtime_error("bad shape to make structured type");
        const auto unit_ = unit_or_default(unit, dtype<T>);
        auto var = variable::make_structures<T, Elem>(
            Dimensions(to_dim_type(labels),
                       std::vector<scipp::index>(
                           values.shape(), values.shape() + labels.size())),
            unit_,
            element_array<Elem>(values.size(), core::init_for_overwrite));
        auto elems = var.template elements<T>();
        if constexpr (sizeof...(N) != 1)
          elems = fold(elems, Dim::InternalStructureComponent,
                       Dimensions({Dim::InternalStructureRow,
                                   Dim::InternalStructureColumn},
                                  {scipp::index(N)...}));
        copy_array_into_view(values, elems.template values<Elem>(),
                             elems.dims());
        return var;
      },
      py::arg("dims"), py::arg("values"), py::arg("unit") = DefaultUnit{});
}

template <class T> struct GetElements {
  static auto apply(Variable &var, const std::string &key) {
    return var.elements<T>(key);
  }
};

template <class T> struct SetElements {
  static auto apply(Variable &var, const std::string &key,
                    const Variable &elems) {
    copy(elems, var.elements<T>(key));
  }
};

void bind_init(py::class_<Variable> &cls);

void init_variable(py::module &m) {
  // Needed to let numpy arrays keep alive the scipp buffers.
  // VariableConcept must ALWAYS be passed to Python by its handle.
  py::class_<VariableConcept, VariableConceptHandle> variable_concept(
      m, "_VariableConcept");

  py::class_<Variable> variable(m, "Variable", py::dynamic_attr(),
                                R"(
Array of values with dimension labels and a unit, optionally including an array
of variances.)");

  bind_init(variable);
  variable
      .def("rename_dims", &rename_dims<Variable>, py::arg("dims_dict"),
           py::pos_only(), "Rename dimensions.")
      .def_property_readonly("dtype", &Variable::dtype)
      .def("__sizeof__",
           [](const Variable &self) {
             return size_of(self, SizeofTag::ViewOnly);
           })
      .def("underlying_size", [](const Variable &self) {
        return size_of(self, SizeofTag::Underlying);
      });

  bind_common_operators(variable);

  bind_astype(variable);

  bind_slice_methods(variable);

  bind_comparison<Variable>(variable);
  bind_comparison_scalars(variable);

  bind_in_place_binary<Variable>(variable);
  bind_in_place_binary_scalars(variable);

  bind_binary<Variable>(variable);
  bind_binary<DataArray>(variable);
  bind_binary_scalars(variable);
  bind_reverse_binary_scalars(variable);

  bind_unary(variable);

  bind_boolean_unary(variable);
  bind_logical<Variable>(variable);

  bind_data_properties(variable);

  m.def(
      "islinspace",
      [](const Variable &x,
         const std::optional<std::string> &dim = std::nullopt) {
        return scipp::variable::islinspace(x, dim.has_value() ? Dim{*dim}
                                                              : x.dim());
      },
      py::arg("x"), py::arg("dim") = py::none(),
      py::call_guard<py::gil_scoped_release>());

  bind_structured_creation<Eigen::Vector3d, double, 3>(m, "vectors");
  bind_structured_creation<Eigen::Matrix3d, double, 3, 3>(m, "matrices");
  bind_structured_creation<Eigen::Affine3d, double, 4, 4>(m,
                                                          "affine_transforms");
  bind_structured_creation<scipp::core::Quaternion, double, 4>(m, "rotations");
  bind_structured_creation<scipp::core::Translation, double, 3>(m,
                                                                "translations");

  using structured_t =
      std::tuple<Eigen::Vector3d, Eigen::Matrix3d, Eigen::Affine3d,
                 scipp::core::Quaternion, scipp::core::Translation>;
  m.def("_element_keys", element_keys);
  m.def("_get_elements", [](Variable &self, const std::string &key) {
    return core::callDType<GetElements>(
        structured_t{}, variableFactory().elem_dtype(self), self, key);
  });
  m.def("_set_elements", [](Variable &self, const std::string &key,
                            const Variable &elems) {
    core::callDType<SetElements>(
        structured_t{}, variableFactory().elem_dtype(self), self, key, elems);
  });
}
