// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;

namespace py = pybind11;

// see https://www.reddit.com/r/C_Programming/comments/3dfu5w/is_it_possible_to_modify_each_argument_in_a/

// Make a FOREACH macro
#define FE_1(WHAT, X) WHAT(X) 
#define FE_2(WHAT, X, ...) WHAT(X)FE_1(WHAT, __VA_ARGS__)
#define FE_3(WHAT, X, ...) WHAT(X)FE_2(WHAT, __VA_ARGS__)
#define FE_4(WHAT, X, ...) WHAT(X)FE_3(WHAT, __VA_ARGS__)
#define FE_5(WHAT, X, ...) WHAT(X)FE_4(WHAT, __VA_ARGS__)
//... repeat as needed
#define GET_MACRO(_1,_2,_3,_4,_5,NAME,...) NAME 
#define FOR_EACH(action,...) \
  GET_MACRO(__VA_ARGS__,FE_5,FE_4,FE_3,FE_2,FE_1)(action,__VA_ARGS__)

#define PYARG_FORMAT(IN) ,py::arg(IN)
#define PYARGS(...) FOR_EACH(PYARG_FORMAT, __VA_ARGS__)

// DEF_STRUCT(myStruct, a = "first",
//                      b = "second",
//                      c = "third")

// result after preprocessing:
// extern const struct myStruct { char *a = "first";char *b = "second";char *c = "third"; };

// // #define FREE_FUNCTION_PYARGS(args) ##args

// #define FREE_FUNCTION_ARGS(op, mod, T)                                \


// #define BIND_FREE_FUNCTION(op, mod, pyargs, docstring, )                                \
//   mod.def(#op, [](T::const_view_type self) { return op(self); },
//           FREE_FUNCTION_PYARGS(pyargs), \
//           py::call_guard<py::gil_scoped_release>(),                            \
//           docstring);                                        \



// __VA_ARGS__

// template<class T1, class T2>
// void bind_free_function(std::string name, py::module &m, std::string description,
//   std::map<std::string, std::string> &params,
//   std::string raises, std::string seealso, std::string returns, std::string rtype) {
//   // Construct docstring
//   std::string docstring = description + "\n";
//   std::string pyargs;
//   for (auto &elem : params) {
//     docstring += ":param " + elem.first + ": " + elem.second +"\n";
//     pyargs += "py:arg(\"" + elem.first + "\"), "
//   }
//   docstring += ":raises: " + raises + "\n";
//   docstring += ":seealso: " + seealso + "\n";
//   docstring += ":return: " + returns + "\n";
//   docstring += ":rtype: " + rtype;

//   BIND_FREE_FUNCTION(op, mod, T)

//     m.def(name, py::overload_cast<ConstView, const Dim>(&flatten),
//         py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
//         R"(
//         Flatten the specified dimension into event lists, equivalent to summing dense data.

//         :param x: Variable, DataArray, or Dataset to flatten.
//         :param dim: Dimension over which to flatten.
//         :raises: If the dimension does not exist, or if x does not contain event lists
//         :seealso: :py:class:`scipp.sum`
//         :return: New variable, data array, or dataset containing the flattened data.
//         :rtype: Variable, DataArray, or Dataset)");

// }


// template <class T> void bind_flatten(py::module &m) {
//   using ConstView = const typename T::const_view_type &;
//   bind_free_function<T, ConstView, const Dim>(
//     "flatten",
//     m,
//     {{"x", "Variable, DataArray, or Dataset to flatten"},
//      {"dim", "Dimension over which to flatten"}},
//     "If the dimension does not exist, or if x does not contain event lists",
//     ":py:class:`scipp.sum`",
//     "New variable, data array, or dataset containing the flattened data.",
//     "Variable, DataArray, or Dataset"
//     )
//   BIND_FREE_FUNCTION()
//   m.def("flatten", py::overload_cast<ConstView, const Dim>(&flatten),
//         py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
//         R"(
//         Flatten the specified dimension into event lists, equivalent to summing dense data.

//         :param x: Variable, DataArray, or Dataset to flatten.
//         :param dim: Dimension over which to flatten.
//         :raises: If the dimension does not exist, or if x does not contain event lists
//         :seealso: :py:class:`scipp.sum`
//         :return: New variable, data array, or dataset containing the flattened data.
//         :rtype: Variable, DataArray, or Dataset)");
// }

template <class T> void bind_flatten(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  m.def("flatten", py::overload_cast<ConstView, const Dim>(&flatten),
        py::call_guard<py::gil_scoped_release>(),
        R"(
        Flatten the specified dimension into event lists, equivalent to summing dense data.

        :param x: Variable, DataArray, or Dataset to flatten.
        :param dim: Dimension over which to flatten.
        :raises: If the dimension does not exist, or if x does not contain event lists
        :seealso: :py:class:`scipp.sum`
        :return: New variable, data array, or dataset containing the flattened data.
        :rtype: Variable, DataArray, or Dataset)"
        // py::arg("x"), py::arg("dim"));
        PYARGS("x", "dim"));
}

template <class T> void bind_concatenate(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  m.def("concatenate", py::overload_cast<ConstView, ConstView, const Dim>(&concatenate),
        py::arg("x"), py::arg("y"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
        R"(
        Concatenate input variables, or all the variables in a supplied dataset, along the given dimension.

        Concatenation can happen in two ways:
        - Along an existing dimension, yielding a new dimension extent given by the sum of the input's extents.
        - Along a new dimension that is not contained in either of the inputs, yielding an output with one extra dimensions.

        In the case of a dataset or data array, data, coords, and masks are concatenated.
        Coords, and masks for any but the given dimension are required to match and are copied to the output without changes.
        In the case of a dataset, the output contains only items that are present in both inputs.

        :param x: First Variable, DataArray, or Dataset.
        :param y: Second Variable, DataArray, or Dataset.
        :param dim: Dimension over which to concatenate.
        :raises: If the dtype or unit does not match, or if the dimensions and shapes are incompatible.
        :return: New variable, data array, or dataset containing the concatenated data.
        :rtype: Variable, DataArray, or Dataset)");
}

template <class T> void bind_abs(py::module &m) {

  using ConstView = const typename T::const_view_type &;
  using View = const typename T::view_type &;
  const std::string description = R"(
        Element-wise absolute value.)";

  m.def("abs", [](ConstView self) { return abs(self); },
        py::arg("x"), py::call_guard<py::gil_scoped_release>(),
        &((description + R"(
        :param x: Input Variable, DataArray, or Dataset.
        :raises: If the dtype has no absolute value, e.g., if it is a string
        :seealso: :py:class:`scipp.norm` for vector-like dtype
        :return: Copy of the input with values replaced by the absolute values
        :rtype: Variable, DataArray, or Dataset)")[0]));
  m.def("abs", [](ConstView self, View out) {
          return abs(self, out);
        },
        py::arg("x"), py::arg("out"), py::call_guard<py::gil_scoped_release>(),
        &((description + R"( (in-place))" +
        R"(
        :param x: Input Variable, DataArray, or Dataset.
        :param out: Output buffer to which the absolute values will be written.
        :raises: If the dtype has no absolute value, e.g., if it is a string.
        :seealso: :py:class:`scipp.norm` for vector-like dtype.
        :return: Input with values replaced by the absolute values.
        :rtype: VariableView, DataArrayView, or DatasetView)")[0]));
}


void init_operations(py::module &m) {
  bind_flatten<Variable>(m);
  bind_flatten<DataArray>(m);
  bind_flatten<Dataset>(m);

  bind_concatenate<Variable>(m);
  bind_concatenate<DataArray>(m);
  bind_concatenate<Dataset>(m);

  bind_abs<Variable>(m);
}
