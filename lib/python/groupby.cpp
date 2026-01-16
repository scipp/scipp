// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/dataset/groupby.h"
#include "scipp/dataset/dataset.h"

#include "docstring.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::dataset;

namespace py = pybind11;

template <class T> Docstring docstring_groupby_numeric(const std::string &op) {
  return Docstring()
      .description("Element-wise " + op +
                   " over the specified dimension "
                   "within a group.")
      .returns("The computed " + op +
               " over each group, combined along "
               "the dimension specified when calling :py:func:`scipp.groupby`.")
      .rtype<T>()
      .param("dim", "Dimension to reduce when computing the " + op + ".", "Dim")
      .examples(R"(
Group by label coordinate:

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0]),
  ...     coords={'label': sc.array(dims=['x'], values=['a', 'b', 'a', 'b'])}
  ... )
  >>> grouped = da.groupby('label').)" +
                op + R"(('x')
  >>> grouped.sizes
  {'label': 2}

Group by bin edges:

  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0]),
  ...     coords={'x': sc.array(dims=['x'], values=[0.5, 1.5, 2.5, 3.5])}
  ... )
  >>> bins = sc.array(dims=['x'], values=[0.0, 2.0, 4.0])
  >>> grouped = da.groupby('x', bins=bins).)" +
                op + R"(('x')
  >>> grouped.sizes
  {'x': 2}
)");
}

template <class T> Docstring docstring_groupby_bool(const std::string &op) {
  return Docstring()
      .description("Element-wise " + op +
                   " over the specified dimension "
                   "within a group.")
      .returns("The computed " + op +
               " over each group, combined along "
               "the dimension specified when calling :py:func:`scipp.groupby`.")
      .rtype<T>()
      .param("dim", "Dimension to reduce when computing the " + op + ".", "Dim")
      .examples(R"(
Group by label coordinate:

  >>> import scipp as sc
  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[True, False, True, True]),
  ...     coords={'label': sc.array(dims=['x'], values=['a', 'b', 'a', 'b'])}
  ... )
  >>> grouped = da.groupby('label').)" +
                op + R"(('x')
  >>> grouped.sizes
  {'label': 2}

Group by bin edges:

  >>> da = sc.DataArray(
  ...     sc.array(dims=['x'], values=[True, False, True, True]),
  ...     coords={'x': sc.array(dims=['x'], values=[0.5, 1.5, 2.5, 3.5])}
  ... )
  >>> bins = sc.array(dims=['x'], values=[0.0, 2.0, 4.0])
  >>> grouped = da.groupby('x', bins=bins).)" +
                op + R"(('x')
  >>> grouped.sizes
  {'x': 2}
)");
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define BIND_GROUPBY_OP_NUMERIC(CLS, NAME)                                     \
  CLS.def(                                                                     \
      TOSTRING(NAME),                                                          \
      [](const GroupBy<T> &self, const std::string &dim) {                     \
        return self.NAME(Dim{dim});                                            \
      },                                                                       \
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),                \
      docstring_groupby_numeric<T>(TOSTRING(NAME)).c_str());

#define BIND_GROUPBY_OP_BOOL(CLS, NAME)                                        \
  CLS.def(                                                                     \
      TOSTRING(NAME),                                                          \
      [](const GroupBy<T> &self, const std::string &dim) {                     \
        return self.NAME(Dim{dim});                                            \
      },                                                                       \
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),                \
      docstring_groupby_bool<T>(TOSTRING(NAME)).c_str());

template <class T> void bind_groupby(py::module &m, const std::string &name) {
  m.def(
      "groupby",
      [](const T &x, const std::string &dim) { return groupby(x, Dim{dim}); },
      py::arg("data"), py::arg("group"),
      py::call_guard<py::gil_scoped_release>());

  m.def(
      "groupby",
      [](const T &x, const std::string &dim, const Variable &bins) {
        return groupby(x, Dim{dim}, bins);
      },
      py::arg("data"), py::arg("group"), py::arg("bins"),
      py::call_guard<py::gil_scoped_release>());

  m.def("groupby",
        py::overload_cast<const T &, const Variable &, const Variable &>(
            &groupby),
        py::arg("data"), py::arg("group"), py::arg("bins"),
        py::call_guard<py::gil_scoped_release>());

  py::class_<GroupBy<T>> groupBy(m, name.c_str(), R"(
    GroupBy object implementing split-apply-combine mechanism.)");

  BIND_GROUPBY_OP_NUMERIC(groupBy, mean);
  BIND_GROUPBY_OP_NUMERIC(groupBy, sum);
  BIND_GROUPBY_OP_NUMERIC(groupBy, nansum);
  BIND_GROUPBY_OP_BOOL(groupBy, all);
  BIND_GROUPBY_OP_BOOL(groupBy, any);
  BIND_GROUPBY_OP_NUMERIC(groupBy, min);
  BIND_GROUPBY_OP_NUMERIC(groupBy, nanmin);
  BIND_GROUPBY_OP_NUMERIC(groupBy, max);
  BIND_GROUPBY_OP_NUMERIC(groupBy, nanmax);
  groupBy.def(
      "concat",
      [](const GroupBy<T> &self, const std::string &dim) {
        return self.concat(Dim{dim});
      },
      py::arg("dim"), py::call_guard<py::gil_scoped_release>(),
      R"(Concatenate bins within each group.

This operation is used with binned data to combine events from different
bins that belong to the same group. The dimension being reduced (specified
by the dim argument) is removed, and the events from all bins along that
dimension within each group are concatenated.

Parameters
----------
dim:
    Dimension to reduce by concatenating bins.

Returns
-------
:
    A DataArray or Dataset with concatenated binned data for each group.

Examples
--------
Concatenate binned data within groups:

  >>> import scipp as sc
  >>> table = sc.data.table_xyz(100)
  >>> binned = table.bin(x=4)
  >>> binned.coords['group'] = sc.array(dims=['x'], values=['A', 'B', 'A', 'B'])
  >>> grouped = binned.groupby('group').concat('x')
  >>> grouped.sizes
  {'group': 2}

The result contains binned data where events from bins 0 and 2 (group 'A')
are concatenated into one bin, and events from bins 1 and 3 (group 'B')
are concatenated into another bin.
)");
}

void init_groupby(py::module &m) {
  bind_groupby<DataArray>(m, "GroupByDataArray");
  bind_groupby<Dataset>(m, "GroupByDataset");
}
