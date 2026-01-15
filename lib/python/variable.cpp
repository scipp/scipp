// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/units/unit.h"

#include "scipp/core/spatial_transforms.h"
#include "scipp/core/time_point.h"

#include "scipp/variable/operations.h"
#include "scipp/variable/structures.h"
#include "scipp/variable/util.h"
#include "scipp/variable/variable.h"
#include "scipp/variable/variable_factory.h"

#include "scipp/dataset/dataset.h"

#include "bind_data_access.h"
#include "bind_operators.h"
#include "bind_slice_methods.h"
#include "dim.h"
#include "pybind11.h"
#include "rename.h"

using namespace scipp;
using namespace scipp::variable;

namespace py = pybind11;

template <class T> struct GetElements {
  static Variable apply(Variable &var, const std::string &key) {
    return var.elements<T>(key);
  }
};

template <class T> struct SetElements {
  static void apply(Variable &var, const std::string &key,
                    const Variable &elems) {
    copy(elems, var.elements<T>(key));
  }
};

template <class T> void bind_alignment_functions(py::class_<T> &variable) {
  // We use a separate setter instead of making the 'aligned' property writable
  // in order to reduce the chance of accidentally setting the flag on
  // temporary variables.
  variable.def_property_readonly(
      "aligned", [](const Variable &self) { return self.is_aligned(); },
      R"(Alignment flag for coordinates.

Indicates whether a coordinate is aligned.
Aligned coordinates must match between the operands of binary operations while
unaligned coordinates are dropped on mismatch.

This flag is only meaningful when the variable is contained in a ``coords``
``dict``.

It cannot be set on a variable directly;
instead, use :meth:`sc.Coords.set_aligned`.

For *binned* coordinates of a binned data array ``da``,
``da.bins.coords[name].aligned`` should always be ``True``.
The alignment w.r.t. the events can be queried via
``da.bins.coords[name].bins.aligned`` and set via
``da.bins.coords.set_aligned(name, aligned)``.
)");
}

void bind_init(py::class_<Variable> &cls);

void init_variable(py::module &m) {
  // Needed to let numpy arrays keep alive the scipp buffers.
  // VariableConcept must ALWAYS be passed to Python by its handle.
  py::class_<VariableConcept, VariableConceptHandle> variable_concept(
      m, "_VariableConcept");

  py::class_<Variable> variable(m, "Variable", py::dynamic_attr(),
                                R"(
Array of values with dimension labels and a unit, optionally including an array
of variances.

Variables support NumPy-like indexing and slicing with dimension labels:

Examples
--------
Create a variable and access elements:

  >>> import scipp as sc
  >>> var = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0], unit='m')

Integer indexing for 1-D variables (implicit dimension):

  >>> var[0]
  <scipp.Variable> ()    float64              [m]  1
  >>> var[-1]
  <scipp.Variable> ()    float64              [m]  4

Slicing:

  >>> var[1:3]
  <scipp.Variable> (x: 2)    float64              [m]  [2, 3]
  >>> var[::2]
  <scipp.Variable> (x: 2)    float64              [m]  [1, 3]

For multi-dimensional variables, use explicit dimension labels:

  >>> var2d = sc.array(dims=['x', 'y'], values=[[1, 2, 3], [4, 5, 6]])
  >>> var2d['x', 0]
  <scipp.Variable> (y: 3)      int64  [dimensionless]  [1, 2, 3]
  >>> var2d['y', 1:3].sizes
  {'x': 2, 'y': 2}

Setting values via indexing:

  >>> var['x', 0] = sc.scalar(10.0, unit='m')
  >>> var
  <scipp.Variable> (x: 4)    float64              [m]  [10, 2, 3, 4]

See Also
--------
scipp.array, scipp.scalar
)");

  bind_init(variable);
  variable.def("_rename_dims", &rename_dims<Variable>)
      .def_property_readonly("dtype", &Variable::dtype);

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
  bind_alignment_functions(variable);

  m.def(
      "islinspace",
      [](const Variable &x,
         const std::optional<std::string> &dim = std::nullopt) {
        return scipp::variable::islinspace(x, dim.has_value() ? Dim{*dim}
                                                              : x.dim());
      },
      py::arg("x"), py::arg("dim") = py::none(),
      py::call_guard<py::gil_scoped_release>());

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
