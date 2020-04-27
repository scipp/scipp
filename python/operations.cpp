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

namespace py = pybind11;

template <class T> void bind_flatten(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  bind_free_function<T, ConstView, const Dim>(
    // Operation function pointer
    flatten,
    // Operation python name
    "flatten",
    // py::module
    m,
    // Input parameters
    {"x", "Variable, DataArray, or Dataset to flatten."},
    {"dim", "Dimension over which to flatten."},
    // Descrption
    "Flatten the specified dimension into event lists, equivalent to summing dense data.",
    // Raises
    "If the dimension does not exist, or if x does not contain event lists.",
    // See also
    ":py:class:`scipp.sum`",
    // Returns
    "New variable, data array, or dataset containing the flattened data.",
    // Return type
    "Variable, DataArray, or Dataset.");
}

template <class T> void bind_concatenate(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  bind_free_function<T, ConstView, ConstView, const Dim>(
    // Operation function pointer
    concatenate,
    // Operation python name
    "concatenate",
    // py::module
    m,
    // Input parameters
    {"x", "First Variable, DataArray, or Dataset."},
    {"y", "Second Variable, DataArray, or Dataset."},
    {"dim", "Dimension over which to concatenate."},
    // Descrption
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
    "Variable, DataArray, or Dataset.");
}



template <class T> void bind_abs(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  using View = typename T::view_type;
  bind_free_function_with_out<T, View, ConstView>(
    // Operation function pointer
    abs,
    // In-place operation function pointer
    abs,
    // Operation python name
    "abs",
    // py::module
    m,
    // Input parameters
    {"x", "Input Variable, DataArray, or Dataset."},
    // Descrption
    "Element-wise absolute value.",
    // Raises
    "If the dtype has no absolute value, e.g., if it is a string",
    // See also
    ":py:class:`scipp.norm` for vector-like dtype",
    // Returns
    "Variable, data array, or dataset containing the absolute values.",
    // Return type
    "Variable, DataArray, or Dataset.");
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
