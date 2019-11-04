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

  neutron.def("source_position", source_position, R"(
    Extract the neutron source position from a dataset.

    :return: A scalar variable containing the source position.
    :rtype: Variable)");

  neutron.def("sample_position", sample_position, R"(
    Extract the sample position from a dataset.

    :return: A scalar variable containing the sample position.
    :rtype: Variable)");

  neutron.def("l1", l1, R"(
    Compute L1, the length of the primary flight path (distance between neutron source and sample) from a dataset.

    :return: A scalar variable containing L1.
    :rtype: Variable)");

  neutron.def("l2", l2, R"(
    Compute L2, the length of the secondary flight paths (distances between sample and detector pixels) from a dataset.

    :return: A variable containing L2 for all detector pixels.
    :rtype: Variable)");

  neutron.def("scattering_angle", scattering_angle, R"(
    Compute :math:`\theta`, the scattering angle in Bragg's law, from a dataset.

    :return: A variable containing :math:`\theta` for all detector pixels.
    :rtype: Variable)");

  neutron.def("two_theta", two_theta, R"(
    Compute :math:`2\theta`, twice the scattering angle in Bragg's law, from a dataset.

    :return: A variable containing :math:`2\theta` for all detector pixels.
    :rtype: Variable)");
}
