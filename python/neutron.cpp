// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/dataset/dataset.h"
#include "scipp/neutron/beamline.h"
#include "scipp/neutron/convert.h"
#include "scipp/neutron/diffraction/convert_with_calibration.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::neutron;

namespace py = pybind11;

template <class T> void bind_positions(py::module &m) {
  m.def("position", py::overload_cast<T>(position), R"(
    Extract the detector pixel positions from a data array or a dataset.

    :return: A variable containing the detector pixel positions.
    :rtype: Variable)");

  m.def("source_position", py::overload_cast<T>(source_position), R"(
    Extract the neutron source position from a data array or a dataset.

    :return: A scalar variable containing the source position.
    :rtype: Variable)");

  m.def("sample_position", py::overload_cast<T>(sample_position), R"(
    Extract the sample position from a data array or a dataset.

    :return: A scalar variable containing the sample position.
    :rtype: Variable)");
}

template <class T> void bind_beamline(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  using View = const typename T::view_type &;

  bind_positions<View>(m);
  bind_positions<ConstView>(m);

  m.def("flight_path_length", py::overload_cast<ConstView>(flight_path_length),
        R"(
    Compute the length of the total flight path from a data array or a dataset.

    If a sample position is found this is the sum of `l1` and `l2`, otherwise the distance from the source.

    :return: A scalar variable containing the total length of the flight path.
    :rtype: Variable)");

  m.def("l1", py::overload_cast<ConstView>(l1), R"(
    Compute L1, the length of the primary flight path (distance between neutron source and sample) from a data array or a dataset.

    :return: A scalar variable containing L1.
    :rtype: Variable)");

  m.def("l2", py::overload_cast<ConstView>(l2), R"(
    Compute L2, the length of the secondary flight paths (distances between sample and detector pixels) from a data array or a dataset.

    :return: A variable containing L2 for all detector pixels.
    :rtype: Variable)");

  m.def("scattering_angle", py::overload_cast<ConstView>(scattering_angle), R"(
    Compute :math:`\theta`, the scattering angle in Bragg's law, from a data array or a dataset.

    :return: A variable containing :math:`\theta` for all detector pixels.
    :rtype: Variable)");

  m.def("two_theta", py::overload_cast<ConstView>(two_theta), R"(
    Compute :math:`2\theta`, twice the scattering angle in Bragg's law, from a data array or a dataset.

    :return: A variable containing :math:`2\theta` for all detector pixels.
    :rtype: Variable)");
}

namespace {
auto realign_flag(const py::object &obj) {
  if (obj.is_none())
    return ConvertRealign::None;
  const auto &r = obj.cast<std::string>();
  if (r == "linear")
    return ConvertRealign::Linear;
  else
    throw std::runtime_error(
        "Allowed values for `realign` are: None, 'linear'");
}
} // namespace

template <class T> void bind_convert(py::module &m) {
  using ConstView = const typename T::const_view_type &;
  const char *doc = R"(
    Convert dimension (unit) into another.

    Currently only conversion from time-of-flight (Dim.Tof) to other time-of-flight-derived units such as d-spacing (Dim.DSpacing) is supported.

    :param data: Input data with time-of-flight dimension (Dim.Tof)
    :param from: Dimension to convert from
    :param to: Dimension to convert into
    :param out: Optional output container
    :param realign: Optionally realign realigned data to keep 1D coords, allowed values: None, 'linear'
    :return: New data array or dataset with converted dimension (dimension labels, coordinate values, and units)
    :rtype: DataArray or Dataset)";
  m.def(
      "convert",
      [](ConstView data, const Dim from, const Dim to,
         const py::object &realign_obj) {
        return py::cast(convert(data, from, to, realign_flag(realign_obj)));
      },
      py::arg("data"), py::arg("from"), py::arg("to"),
      py::arg("realign") = py::none(), py::call_guard<py::gil_scoped_release>(),
      doc);
  m.def(
      "convert",
      [](py::object &obj, const Dim from, const Dim to, T &out,
         const py::object &realign_obj) {
        auto &data = obj.cast<T &>();
        if (&data != &out)
          throw std::runtime_error("Currently only out=<input> is supported");
        data = convert(std::move(data), from, to, realign_flag(realign_obj));
        return obj;
      },
      py::arg("data"), py::arg("from"), py::arg("to"), py::arg("out"),
      py::arg("realign") = py::none(), py::call_guard<py::gil_scoped_release>(),
      doc);
}

template <class T> void bind_convert_with_calibration(py::module &m) {
  const char *doc = R"(
    Convert unit of powder-diffraction data based on calibration.

    :param data: Input data with time-of-flight dimension (Dim.Tof)
    :param calibration: Table of calibration constants
    :param out: Optional output container
    :return: New data array or dataset with time-of-flight converted to d-spacing (Dim.DSpacing)
    :rtype: DataArray or Dataset

    .. seealso:: Use :py:func:`scipp.neutron.convert` for unit conversion based on beamline-geometry information instead of calibration information.)";
  m.def("convert_with_calibration",
        py::overload_cast<T, dataset::Dataset>(
            diffraction::convert_with_calibration),
        py::arg("data"), py::arg("calibration"),
        py::call_guard<py::gil_scoped_release>(), doc);
  m.def(
      "convert_with_calibration",
      [](py::object &obj, const dataset::Dataset &calibration, const T &out) {
        auto &data = obj.cast<T &>();
        if (&data != &out)
          throw std::runtime_error("Currently only out=<input> is supported");
        data =
            diffraction::convert_with_calibration(std::move(data), calibration);
        return obj;
      },
      py::arg("data"), py::arg("calibration"), py::arg("out"),
      py::call_guard<py::gil_scoped_release>(), doc);
}

void bind_diffraction(py::module &m) {
  auto diffraction = m.def_submodule("neutron_diffraction");
  bind_convert_with_calibration<dataset::DataArray>(diffraction);
  bind_convert_with_calibration<dataset::Dataset>(diffraction);
}

void init_neutron(py::module &m) {
  auto neutron = m.def_submodule("neutron");

  bind_convert<dataset::DataArray>(neutron);
  bind_convert<dataset::Dataset>(neutron);
  bind_beamline<dataset::DataArray>(neutron);
  bind_beamline<dataset::Dataset>(neutron);

  // This is deliberately `m` and not `neutron` due to how nested imports work
  // in Python in combination with mixed C++/Python modules.
  bind_diffraction(m);
}
