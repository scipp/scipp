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

template <class T> void bind_mean(py::module &m, bool out_arg = false) {
  using ConstView = const typename T::const_view_type &;
  const Docstring docs = {
    // Description
    R"(
    Element-wise mean over the specified dimension, if variances are
    present, the new variance is computated as standard-deviation of the
    mean.

    If the input has variances, the variances stored in the ouput are based
    on the "standard deviation of the mean", i.e.,
    :math:`\sigma_{mean} = \sigma / \sqrt{N}`.
    :math:`N` is the length of the input dimension.
    :math:`\sigma` is estimated as the average of the standard deviations of
    the input elements along that dimension.
    This assumes that elements follow a normal distribution.)",
    // Raises
    "If the dimension does not exist, or the dtype cannot be summed, e.g., if it is a string.",
    // See also
    ":py:class:`scipp.sum`",
    // Returns
    "New variable, data array, or dataset containing the mean.",
    // Return type
    "Variable, DataArray, or Dataset.",
    // Parameters/arguments
    {{"x", "Data to calculate mean of."},
     {"dim", "Dimension over which to calculate mean."}}
  };

  // strpair param1 = {"x", "Data to calculate mean of."};
  // strpair param2 = {"dim", "Dimension over which to calculate mean."};

  bind_free_function<T, ConstView, const Dim>(
    mean,
    "mean",
    m,
    docs);
    // param1,
    // param2,

    // docs.description,
    // docs.raises,
    // docs.seealso,
    // docs.returns,
    // docs.rtype);

  // if(out_arg) {
  //   using View = typename T::view_type;
  //   bind_free_function<View, ConstView, const Dim, const View &>(
  //   mean,
  //   "mean",
  //   m,
  //   param1,
  //   param2,
  //   {"out", "Output buffer."},
  //   docs.description + " (in-place)",
  //   docs.raises,
  //   docs.seealso,
  //   docs.returns,
  //   docs.rtype + " (View)");
  // }

  // bind_free_function<T, ConstView, const Dim>(
  //   // Operation function pointer
  //   mean,
  //   // Operation python name
  //   "mean",
  //   // py::module
  //   m,
  //   // Input parameters
  //   {"x", "Data to calculate mean of."},
  //   {"dim", "Dimension over which to calculate mean."},
  //   // Description
  //   R"(
  //   Element-wise mean over the specified dimension, if variances are
  //   present, the new variance is computated as standard-deviation of the
  //   mean.

  //   If the input has variances, the variances stored in the ouput are based
  //   on the "standard deviation of the mean", i.e.,
  //   :math:`\sigma_{mean} = \sigma / \sqrt{N}`.
  //   :math:`N` is the length of the input dimension.
  //   :math:`\sigma` is estimated as the average of the standard deviations of
  //   the input elements along that dimension.
  //   This assumes that elements follow a normal distribution.)",
  //   // Raises
  //   "If the dimension does not exist, or the dtype cannot be summed, e.g., if it is a string.",
  //   // See also
  //   ":py:class:`scipp.sum`",
  //   // Returns
  //   "New variable, data array, or dataset containing the mean.",
  //   // Return type
  //   "Variable, DataArray, or Dataset.");

  // // using View = const typename T::view_type &;
  // const std::string description = R"(
  //       Element-wise mean over the specified dimension, if variances are
  //       present, the new variance is computated as standard-deviation of the
  //       mean.

  //       If the input has variances, the variances stored in the ouput are based
  //       on the "standard deviation of the mean", i.e.,
  //       :math:`\sigma_{mean} = \sigma / \sqrt{N}`.
  //       :math:`N` is the length of the input dimension.
  //       :math:`\sigma` is estimated as the average of the standard deviations of
  //       the input elements along that dimension.
  //       This assumes that elements follow a normal distribution.)";
  // m.def("mean", py::overload_cast<ConstView, const Dim>(&mean),
  //       py::arg("x"), py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
  //       &((description + R"(
  //       :param x: Data to calculate mean of.
  //       :param dim: Dimension over which to calculate mean.
  //       :raises: If the dimension does not exist, or the dtype cannot be summed,
  //        e.g., if it is a string
  //       :seealso: :py:class:`scipp.sum`
  //       :return: New variable, data array, or dataset containing the mean.
  //       :rtype: Variable, DataArray, or Dataset)")[0]));
}

template <class T> void bind_sum(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  bind_free_function<T, ConstView, const Dim>(sum, "sum", m,
    {"x", "Data to sum."},
    {"dim", "Dimension over which to sum."},
    "Element-wise sum over the specified dimension.",
    "If the dimension does not exist, or if the dtype cannot be summed, e.g., if it is a string.",
    ":py:class:`scipp.mean`",
    "New variable, data array, or dataset containing the sum.",
    "Variable, DataArray, or Dataset.");
}

void init_reduction(py::module &m) {
  bind_mean<Variable>(m);
  bind_mean<DataArray>(m);
  bind_mean<Dataset>(m);

  bind_sum<Variable>(m);
  bind_sum<DataArray>(m);
  bind_sum<Dataset>(m);
}
