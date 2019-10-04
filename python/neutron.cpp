// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/dataset.h"
#include "scipp/neutron/beamline.h"
#include "scipp/neutron/convert.h"

#include "pybind11.h"

using namespace scipp;
using namespace scipp::neutron;
using namespace scipp::neutron::diffraction;

namespace py = pybind11;

void init_neutron(py::module &m) {
  auto neutron = m.def_submodule("neutron");
  auto diffraction = neutron.def_submodule("diffraction");

  diffraction.def("convert_with_calibration", convert_with_calibration, R"(
    Calibration of diffraction data using calibration data.

    Inputs a sparse tof variable and a dataset with calibration parameters

    :return: New dataset with converted TOF to d-spacing)");

  neutron.def("convert", convert, py::call_guard<py::gil_scoped_release>(),
              R"(
    Convert dimension (unit) into another.

    Currently only conversion from time-of-flight (Dim.Tof) to other time-of-flight-derived units such as d-spacing (Dim.DSpacing) is supported.

    :return: New dataset with converted dimension (dimension labels, coordinate values, and units)
    :rtype: Dataset)");

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
