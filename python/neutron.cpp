// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/neutron/beamline.h"
#include "scipp/neutron/convert.h"
#include "scipp/neutron/diffraction/convert_with_calibration.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::neutron;

namespace py = pybind11;

template <class T> void bind_beamline(py::module &m) {
  using View = const typename T::const_view_type &;

  m.def("position", py::overload_cast<View>(position), R"(
    Extract the detector pixel positions from a data array or a dataset.

    :return: A variable containing the detector pixel positions.
    :rtype: Variable)");

  m.def("source_position", py::overload_cast<View>(source_position), R"(
    Extract the neutron source position from a data array or a dataset.

    :return: A scalar variable containing the source position.
    :rtype: Variable)");

  m.def("sample_position", py::overload_cast<View>(sample_position), R"(
    Extract the sample position from a data array or a dataset.

    :return: A scalar variable containing the sample position.
    :rtype: Variable)");

  m.def("l1", py::overload_cast<View>(l1), R"(
    Compute L1, the length of the primary flight path (distance between neutron source and sample) from a data array or a dataset.

    :return: A scalar variable containing L1.
    :rtype: Variable)");

  m.def("l2", py::overload_cast<View>(l2), R"(
    Compute L2, the length of the secondary flight paths (distances between sample and detector pixels) from a data array or a dataset.

    :return: A variable containing L2 for all detector pixels.
    :rtype: Variable)");

  m.def("scattering_angle", py::overload_cast<View>(scattering_angle), R"(
    Compute :math:`\theta`, the scattering angle in Bragg's law, from a data array or a dataset.

    :return: A variable containing :math:`\theta` for all detector pixels.
    :rtype: Variable)");

  m.def("two_theta", py::overload_cast<View>(two_theta), R"(
    Compute :math:`2\theta`, twice the scattering angle in Bragg's law, from a data array or a dataset.

    :return: A variable containing :math:`2\theta` for all detector pixels.
    :rtype: Variable)");
}

void bind_convert(py::module &m) {
  const char *doc = R"(
    Convert dimension (unit) into another.

    Currently only conversion from time-of-flight (Dim.Tof) to other time-of-flight-derived units such as d-spacing (Dim.DSpacing) is supported.

    :param data: Input data with time-of-flight dimension (Dim.Tof)
    :param from: Dimension to convert from
    :param to: Dimension to convert into
    :return: New dataset with converted dimension (dimension labels, coordinate values, and units)
    :rtype: Dataset)";
  m.def("convert", convert, py::arg("data"), py::arg("from"), py::arg("to"),
        py::call_guard<py::gil_scoped_release>(), doc);
}

void init_neutron(py::module &m) {
  auto neutron = m.def_submodule("neutron");
  auto diffraction = m.def_submodule("neutron_diffraction");

  diffraction.def("convert_with_calibration",
                  diffraction::convert_with_calibration, py::arg("data"),
                  py::arg("calibration"), R"(
    Convert unit of powder-diffraction data based on calibration.

    :param data: Input data with time-of-flight dimension (Dim.Tof)
    :param calibration: Table of calibration constants
    :return: New dataset with time-of-flight converted to d-spacing (Dim.DSpacing)
    :rtype: Dataset

    .. seealso:: Use :py:func:`scipp.neutron.convert` for unit conversion based on beamline-geometry information instead of calibration information.)");

  bind_convert(neutron);
  bind_beamline<core::DataArray>(neutron);
  bind_beamline<core::Dataset>(neutron);
}
