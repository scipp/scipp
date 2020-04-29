// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"
#include "bind_free_function.h"

#include "scipp/dataset/dataset.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;
using namespace scipp::python;

namespace py = pybind11;



// template <class T> void bind_flatten(py::module &m) {
//   using ConstView = const typename T::const_view_type &;
//   bind_free_function<T, ConstView, const Dim>(
//     // Operation function pointer
//     flatten,
//     // Operation python name
//     "flatten",
//     // py::module
//     m,
//     // Input parameters
//     {"x", "Variable, DataArray, or Dataset to flatten."},
//     {"dim", "Dimension over which to flatten."},
//     // Description
//     "Flatten the specified dimension into event lists, "
//     "equivalent to summing dense data.",
//     // Raises
//     "If the dimension does not exist, or if x does not contain event lists.",
//     // See also
//     ":py:class:`scipp.sum`",
//     // Returns
//     "New variable, data array, or dataset containing the flattened data.",
//     // Return type
//     "Variable, DataArray, or Dataset.");
// }

// template <class T> void bind_concatenate(py::module &m) {
//   using ConstView = const typename T::const_view_type &;
//   bind_free_function<T, ConstView, ConstView, const Dim>(
//     // Operation function pointer
//     concatenate,
//     // Operation python name
//     "concatenate",
//     // py::module
//     m,
//     // Input parameters
//     {"x", "First Variable, DataArray, or Dataset."},
//     {"y", "Second Variable, DataArray, or Dataset."},
//     {"dim", "Dimension over which to concatenate."},
//     // Description
//     R"(
//     Concatenate input variables, or all the variables in a supplied dataset, along the given dimension.

//     Concatenation can happen in two ways:
//     - Along an existing dimension, yielding a new dimension extent given by the sum of the input's extents.
//     - Along a new dimension that is not contained in either of the inputs, yielding an output with one extra dimensions.

//     In the case of a dataset or data array, data, coords, and masks are concatenated.
//     Coords, and masks for any but the given dimension are required to match and are copied to the output without changes.
//     In the case of a dataset, the output contains only items that are present in both inputs.)",
//     // Raises
//     "If the dtype or unit does not match, or if the dimensions and shapes are incompatible.",
//     // See also
//     ":py:class:`scipp.sum`",
//     // Returns
//     "New variable, data array, or dataset containing the concatenated data.",
//     // Return type
//     "Variable, DataArray, or Dataset.");
// }


// const Docstring abs_docstring() {
//   return {
//     // Description
//     "Element-wise absolute value.",
//     // Raises
//     "If the dtype has no absolute value, e.g., if it is a string.",
//     // See also
//     ":py:class:`scipp.norm` for vector-like dtype",
//     // Returns
//     "Variable, data array, or dataset containing the absolute values.",
//     // Return type
//     "Variable, DataArray, or Dataset.",
//     {{"x", "Input Variable, DataArray, or Dataset."}}
//   };
// }

// template <class T> void bind_abs(py::module &m, bool out_arg = false) {
//   using ConstView = const typename T::const_view_type &;
//   // auto docs = make_abs_docstring();
//   // strpair params = {"x", "Input Variable, DataArray, or Dataset."};

//   bind_free_function<T, ConstView>(
//     abs,
//     "abs",
//     m,
//     abs_docstring());

//   // if(out_arg) {
//   //   using View = typename T::view_type;
//   //   bind_free_function<View, ConstView, const View &>(
//   //   abs,
//   //   "abs",
//   //   m,
//   //   params,
//   //   {"out", "Output buffer."},
//   //   docs.description + " (in-place)",
//   //   docs.raises,
//   //   docs.seealso,
//   //   docs.returns,
//   //   docs.rtype + " (View)");
//   // }
// }

// template <class T> void bind_abs_out(py::module &m, bool out_arg = false) {
//   using ConstView = const typename T::const_view_type &;
//   // auto docs = make_abs_docstring();
//   // strpair params = {"x", "Input Variable, DataArray, or Dataset."};

//   bind_free_function<T, ConstView>(
//     abs,
//     "abs",
//     m,
//     abs_docstring());

//   // if(out_arg) {
//   //   using View = typename T::view_type;
//   //   bind_free_function<View, ConstView, const View &>(
//   //   abs,
//   //   "abs",
//   //   m,
//   //   params,
//   //   {"out", "Output buffer."},
//   //   docs.description + " (in-place)",
//   //   docs.raises,
//   //   docs.seealso,
//   //   docs.returns,
//   //   docs.rtype + " (View)");
//   // }
// }


// template <class T> void bind_dot(py::module &m) {
//   using ConstView = const typename T::const_view_type &;
//   bind_free_function<T, ConstView, ConstView>(
//     // Operation function pointer
//     dot,
//     // Operation python name
//     "dot",
//     // py::module
//     m,
//     // Input parameters
//     {"x", "Left operand Variable, DataArray, or Dataset."},
//     {"y", "Right operand Variable, DataArray, or Dataset."},
//     // Description
//     "Element-wise dot-product.",
//     // Raises
//     "If the dtype is not a vector such as :py:class:`scipp.dtype.vector_3_double.`",
//     // See also
//     "",
//     // Returns
//     "Variable, data array, or dataset with scalar elements based on the two inputs.",
//     // Return type
//     "Variable, DataArray, or Dataset.");
// }



template<class T>
void bind_flatten(py::module &m, Docstring docs) {
  using ConstView = const typename T::const_view_type &;
  bind_free_function<T, ConstView, const Dim>(flatten, "flatten", m, docs);
}

template<class T>
void bind_concatenate(py::module &m, Docstring docs) {
  using ConstView = const typename T::const_view_type &;
  bind_free_function<T, ConstView, ConstView, const Dim>(concatenate, "concatenate", m, docs);
}

// template<class T>
// void bind_abs(py::module &m, Docstring docs) {
//   using ConstView = const typename T::const_view_type &;
//   bind_free_function<T, ConstView>(abs, "abs", m, docs);
// }

// template<class T>
// void bind_dot(py::module &m, Docstring docs) {
//   using ConstView = const typename T::const_view_type &;
//   bind_free_function<T, ConstView, ConstView>(dot, "dot", m, docs);
// }


void init_operations(py::module &m) {

  using ConstView = const typename Variable::const_view_type &;
  using View = typename Variable::view_type;
  Docstring docs;

  // // flatten
  docs = {
    // Description
    "Flatten the specified dimension into event lists, "
    "equivalent to summing dense data.",
    // Raises
    "If the dimension does not exist, or if x does not contain event lists.",
    // See also
    ":py:class:`scipp.sum`",
    // Returns
    "New variable, data array, or dataset containing the flattened data.",
    // Return type
    "Variable, DataArray, or Dataset.",
    // Input parameters
    {{"x", "Variable, DataArray, or Dataset to flatten."},
    {"dim", "Dimension over which to flatten."}}
    // {"x", "Variable, DataArray, or Dataset to flatten."},
    // {"dim", "Dimension over which to flatten."}
  };
  bind_flatten<Variable>(m, docs);
  bind_flatten<DataArray>(m, docs);
  bind_flatten<Dataset>(m, docs);
  // bind_free_function<Variable, VAConstView, const Dim>(flatten, "flatten", m, docs);
  // bind_free_function<DataArray, DAConstView, const Dim>(flatten, "flatten", m, docs);
  // bind_free_function<Dataset, DSConstView, const Dim>(flatten, "flatten", m, docs);

  docs = {
    // Description
    R"(
    Concatenate input variables, or all the variables in a supplied dataset, along the given dimension.

    Concatenation can happen in two ways:
    - Along an existing dimension, yielding a new dimension extent given by the sum of the input's extents.
    - Along a new dimension that is not contained in either of the inputs, yielding an output with one extra dimensions.

    In the case of a dataset or data array, data, coords, and masks are concatenated.
    Coords, and masks for any but the given dimension are required to match and are copied to the output without changes.
    In the case of a dataset, the output contains only items that are present in both inputs.)",
    // Raises
    "If the dtype or unit does not match, or if the dimensions and shapes are incompatible.",
    // See also
    ":py:class:`scipp.sum`",
    // Returns
    "New variable, data array, or dataset containing the concatenated data.",
    // Return type
    "Variable, DataArray, or Dataset.",
    // Input parameters
    {    {"x", "First Variable, DataArray, or Dataset."},
    {"y", "Second Variable, DataArray, or Dataset."},
    {"dim", "Dimension over which to concatenate."}}};
  bind_concatenate<Variable>(m, docs);
  bind_concatenate<DataArray>(m, docs);
  bind_concatenate<Dataset>(m, docs);


  // bind_free_function<Variable, VAConstView, VAConstView, const Dim>(concatenate, "concatenate", m, docs);
  // bind_free_function<DataArray, DAConstView, DAConstView, const Dim>(concatenate, "concatenate", m, docs);
  // bind_free_function<Dataset, DSConstView, DSConstView, const Dim>(concatenate, "concatenate", m, docs);

  // bind_abs<Variable>(m, true);

  // abs
  docs = {
    // Description
    "Element-wise absolute value.",
    // Raises
    "If the dtype has no absolute value, e.g., if it is a string.",
    // See also
    ":py:class:`scipp.norm` for vector-like dtype",
    // Returns
    "Variable, data array, or dataset containing the absolute values.",
    // Return type
    "Variable, DataArray, or Dataset.",
    // Parameters
    {{"x", "Input Variable, DataArray, or Dataset."}}
  };
  bind_free_function<Variable, ConstView>(abs, "abs", m, docs);
  bind_free_function<View, ConstView, const View &>(abs, "abs", m, docs.with_out_arg());




    // Description
  docs = {
    "Element-wise dot-product.",
    // Raises
    "If the dtype is not a vector such as :py:class:`scipp.dtype.vector_3_double.`",
    // See also
    "",
    // Returns
    "Variable, data array, or dataset with scalar elements based on the two inputs.",
    // Return type
    "Variable, DataArray, or Dataset.",
    {    {"x", "Left operand Variable, DataArray, or Dataset."},
    {"y", "Right operand Variable, DataArray, or Dataset."}}};
  bind_free_function<Variable, ConstView, ConstView>(dot, "dot", m, docs);
  // bind_free_function<Variable, VAConstView>(dot, "dot", m, docs);


}
