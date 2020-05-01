// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "pybind11.h"
#include "bind_free_function.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/sort.h"
#include "scipp/variable/operations.h"

using namespace scipp;
using namespace scipp::variable;
using namespace scipp::dataset;
using namespace scipp::python;

namespace py = pybind11;



void init_operations(py::module &m) {

  using VAConstView = const typename Variable::const_view_type &;
  using DAConstView = const typename DataArray::const_view_type &;
  using DSConstView = const typename Dataset::const_view_type &;
  using VAView = typename Variable::view_type;
  Docstring docs;

  // Flatten
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
  };
  bind_free_function<Variable, VAConstView, const Dim>(flatten, "flatten", m, docs);
  bind_free_function<DataArray, DAConstView, const Dim>(flatten, "flatten", m, docs);
  bind_free_function<Dataset, DSConstView, const Dim>(flatten, "flatten", m, docs);

  // Concatenate
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
  bind_free_function<Variable, VAConstView, VAConstView, const Dim>(concatenate, "concatenate", m, docs);
  bind_free_function<DataArray, DAConstView, DAConstView, const Dim>(concatenate, "concatenate", m, docs);
  bind_free_function<Dataset, DSConstView, DSConstView, const Dim>(concatenate, "concatenate", m, docs);





  // Dot-product
  docs = {
    "Element-wise dot-product.",
    "If the dtype is not a vector such as :py:class:`scipp.dtype.vector_3_double.`",
    "",
    "Variable, data array, or dataset with scalar elements based on the two inputs.",
    "Variable, DataArray, or Dataset.",
    {    {"x", "Left operand Variable, DataArray, or Dataset."},
    {"y", "Right operand Variable, DataArray, or Dataset."}}};
  bind_free_function<Variable, VAConstView, VAConstView>(dot, "dot", m, docs);


  // Sort
  docs = {
    "Sort variable along a dimension by a sort key.",
    "If the key is invalid, e.g., if it has not exactly one dimension, or if its dtype is not sortable.",
    "",
    "Sorted variable, data array, or dataset.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Variable, DataArray, or Dataset to be sorted."},
     {"key", "Variable, DataArray, or Dataset as sorting key."}}};
  bind_free_function<Variable, VAConstView, VAConstView>(sort, "sort", m, docs);
  bind_free_function<DataArray, DAConstView, VAConstView>(sort, "sort", m, docs);
  bind_free_function<Dataset, DSConstView, VAConstView>(sort, "sort", m, docs);
  docs.set_description("Sort data array along a dimension by the coordinate values for that dimension.");
  docs.set_raises("If the key is invalid, e.g., if it is not a Dim.");
  docs.set_param(1, {"dim", "Dimension over which to sort."});
  bind_free_function<DataArray, DAConstView, const Dim &>(sort, "sort", m, docs);
  bind_free_function<Dataset, DSConstView, const Dim &>(sort, "sort", m, docs);

  // Absolute value
  docs = {
    "Element-wise absolute value.",
    "If the dtype has no absolute value, e.g., if it is a string.",
    ":py:class:`scipp.norm` for vector-like dtype",
    "Variable, data array, or dataset containing the absolute values.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Input Variable, DataArray, or Dataset."}}
  };
  bind_free_function<Variable, VAConstView>(abs, "abs", m, docs);
  bind_free_function<VAView, VAConstView, const VAView &>(abs, "abs", m, docs.with_out_arg());

  // Square-root
  docs = {
    "Element-wise square-root.",
    "If the dtype has no square-root, e.g., if it is a string.",
    "",
    "Variable, data array, or dataset containing the square-root.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Input Variable, DataArray, or Dataset."}}
  };
  bind_free_function<Variable, VAConstView>(sqrt, "sqrt", m, docs);
  bind_free_function<VAView, VAConstView, const VAView &>(sqrt, "sqrt", m, docs.with_out_arg());


  // Reciprocal
  docs = {
    "Element-wise reciprocal.",
    "",
    "",
    "Variable, data array, or dataset containing the reciprocal of the input values.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Input Variable, DataArray, or Dataset."}}
  };
  bind_free_function<Variable, VAConstView>(reciprocal, "reciprocal", m, docs);
  bind_free_function<DataArray, DAConstView>(reciprocal, "reciprocal", m, docs);
  bind_free_function<VAView, VAConstView, const VAView &>(reciprocal, "reciprocal", m, docs.with_out_arg());


  // Norm
  docs = {
    "Element-wise norm.",
    "If the dtype has no norm, i.e., if it is not a vector.",
    ":py:class:`scipp.abs` for scalar dtype.",
    "Variable, data array, or dataset with scalar elements computed as the norm values if the input elements.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Input Variable, DataArray, or Dataset."}}
  };
  bind_free_function<Variable, VAConstView>(norm, "norm", m, docs);

  // All
  docs = {
    "Element-wise AND over the specified dimension.",
    "If the dimension does not exist, or if the dtype is not bool.",
    ":py:class:`scipp.any`.",
    "Variable, data array, or dataset containing the reduced values.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Input Variable, DataArray, or Dataset."},
     {"dim", "Dimension to reduce."}}
  };
  bind_free_function<Variable, VAConstView, const Dim>(all, "all", m, docs);

  // Any
  docs = {
    "Element-wise OR over the specified dimension.",
    "If the dimension does not exist, or if the dtype is not bool.",
    ":py:class:`scipp.all`.",
    "Variable, data array, or dataset containing the reduced values.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Input Variable, DataArray, or Dataset."},
     {"dim", "Dimension to reduce."}}
  };
  bind_free_function<Variable, VAConstView, const Dim>(any, "any", m, docs);

  // Min
  docs = {
    "Element-wise minimum over the specified dimension.",
    "",
    ":py:class:`scipp.max`.",
    "Variable, data array, or dataset containing the min values.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Input Variable, DataArray, or Dataset."},
     {"dim", "Dimension to reduce."}}
  };
  bind_free_function<Variable, VAConstView, const Dim>(min, "min", m, docs);
  // Function overload ambiguous on number of args, so we expand the docstring.
  bind_free_function<Variable, VAConstView>(min, "min", m, docs.param(0), "Element-wise minimum over all dimensions.",
    docs.raises(), docs.seealso(), docs.returns(), docs.rtype());

  // Max
  docs = {
    "Element-wise maximum over the specified dimension.",
    "",
    ":py:class:`scipp.min`.",
    "Variable, data array, or dataset containing the max values.",
    "Variable, DataArray, or Dataset.",
    {{"x", "Input Variable, DataArray, or Dataset."},
     {"dim", "Dimension to reduce."}}
  };
  bind_free_function<Variable, VAConstView, const Dim>(max, "max", m, docs);
  // Function overload ambiguous on number of args, so we expand the docstring.
  bind_free_function<Variable, VAConstView>(max, "max", m, docs.param(0), "Element-wise maximum over all dimensions.",
    docs.raises(), docs.seealso(), docs.returns(), docs.rtype());



  // Contains_events
  docs = {
    "Return true if the variable contains event data.",
    "",
    "",
    "true or false.",
    "bool.",
    {{"x", "Input Variable or DataArray."}}
  };
  bind_free_function<bool, VAConstView>(contains_events, "contains_events", m, docs);
  bind_free_function<bool, DAConstView>(contains_events, "contains_events", m, docs);


  

  m.def("nan_to_num",
        [](const VariableConstView &self,
           const std::optional<VariableConstView> &nan,
           const std::optional<VariableConstView> &posinf,
           const std::optional<VariableConstView> &neginf) {
          Variable out(self);
          if (nan)
            nan_to_num(out, *nan, out);
          if (posinf)
            positive_inf_to_num(out, *posinf, out);
          if (neginf)
            negative_inf_to_num(out, *neginf, out);
          return out;
        },
        py::call_guard<py::gil_scoped_release>(),
        R"(Element-wise special value replacement

       All elements in the output are identical to input except in the presence of a nan, inf or -inf.
       The function allows replacements to be separately specified for nan, inf or -inf values.
       You can choose to replace a subset of those special values by providing just the required key word arguments.
       If the replacement is value-only and the input has variances,
       the variance at the element(s) undergoing replacement are also replaced with the replacement value.
       If the replacement has a variance and the input has variances,
       the variance at the element(s) undergoing replacement are also replaced with the replacement variance.
       :raises: If the types of input and replacement do not match.
       :return: Input elements are replaced in output with specified subsitutions.
       :rtype: Variable)",
        py::arg("x"), py::arg("nan") = std::optional<VariableConstView>(),
        py::arg("posinf") = std::optional<VariableConstView>(),
        py::arg("neginf") = std::optional<VariableConstView>());

  m.def("nan_to_num",
        [](const VariableConstView &self,
           const std::optional<VariableConstView> &nan,
           const std::optional<VariableConstView> &posinf,
           const std::optional<VariableConstView> &neginf, VariableView &out) {
          if (nan)
            nan_to_num(self, *nan, out);
          if (posinf)
            positive_inf_to_num(self, *posinf, out);
          if (neginf)
            negative_inf_to_num(self, *neginf, out);
          return out;
        },
        py::call_guard<py::gil_scoped_release>(),
        R"(Element-wise special value replacement

       All elements in the output are identical to input except in the presence of a nan, inf or -inf.
       The function allows replacements to be separately specified for nan, inf or -inf values.
       You can choose to replace a subset of those special values by providing just the required key word arguments.
       If the replacement is value-only and the input has variances,
       the variance at the element(s) undergoing replacement are also replaced with the replacement value.
       If the replacement has a variance and the input has variances,
       the variance at the element(s) undergoing replacement are also replaced with the replacement variance.
       :raises: If the types of input and replacement do not match.
       :return: Input elements are replaced in output with specified subsitutions.
       :rtype: Variable)",
        py::arg("x"), py::arg("nan") = std::optional<VariableConstView>(),
        py::arg("posinf") = std::optional<VariableConstView>(),
        py::arg("neginf") = std::optional<VariableConstView>(), py::arg("out"));

}
