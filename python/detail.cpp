// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "detail.h"
#include "pybind11.h"

using namespace scipp;
using namespace scipp::core;

namespace py = pybind11;

void init_detail(py::module &m) {
  py::class_<MoveableVariable>(m, "MoveableVariable");
  py::class_<MoveableDataArray>(m, "MoveableDataArray");

  auto detail = m.def_submodule("detail");

  detail.def(
      "move", [](Variable &var) { return MoveableVariable{std::move(var)}; },
      py::call_guard<py::gil_scoped_release>(), R"(
        This function can be used in a similar way to the C++ std::move to
        transfer ownership of an input Variable to a container.
        This is useful when wanting to avoid unnecessary copies of Variables
        when inserting them into a Dataset.

        :return: A MoveableVariable wrapper class.
        :rtype: MoveableVariable)");

  detail.def(
      "move",
      [](DataArray &data) { return MoveableDataArray{std::move(data)}; },
      py::call_guard<py::gil_scoped_release>(), R"(
        This function can be used in a similar way to the C++ std::move to
        transfer ownership of an input DataArray to a container.
        This is useful when wanting to avoid unnecessary copies of Variables
        when inserting them into a Dataset.

        :return: A MoveableDataArray wrapper class.
        :rtype: MoveableDataArray)");

  detail.def(
      "move_to_data_array",
      [](Variable &data, std::map<Dim, Variable &> &coords,
         std::map<std::string, Variable &> &masks,
         std::map<std::string, Variable &> &attrs, const std::string &name) {
        return DataArray(std::move(data), std::move(coords), std::move(masks),
                         std::move(attrs), name);
      },
      py::arg("data") = Variable{},
      py::arg("coords") = std::map<Dim, Variable>{},
      py::arg("masks") = std::map<std::string, Variable>{},
      py::arg("attrs") = std::map<std::string, Variable>{},
      py::arg("name") = std::string{},
      R"(This functions moves the contents of all the input Variables (data,
      coordinates, masks and attributes) to a new DataArray without
      making copies. Note that after this is called, all variables that were
      passed will be invalidated. This tool should be used with care, and is
      reserved to expert users.

      :return: A DataArray.
      :rtype: DataArray)");
}
