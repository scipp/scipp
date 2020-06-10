// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/variable/operations.h"
#include "scipp/variable/transform.h"

#include "scipp/dataset/dataset.h"

#include "scipp/neutron/beamline.h"

using namespace scipp::variable;
using namespace scipp::dataset;

namespace scipp::neutron {

namespace beamline_impl {

template <class T> static auto position(const T &d) {
  if (d.coords().contains(Dim::Position))
    return d.coords()[Dim::Position];
  else
    return d.attrs()["position"];
}

template <class T> static auto source_position(const T &d) {
  if (d.coords().contains(Dim("source-position")))
    return d.coords()[Dim("source-position")];
  else
    return d.attrs()["source-position"];
}

template <class T> static auto sample_position(const T &d) {
  if (d.coords().contains(Dim("sample-position")))
    return d.coords()[Dim("sample-position")];
  else
    return d.attrs()["sample-position"];
}

template <class T> static Variable flight_path_length(const T &d) {
  // If there is no sample this returns the straight distance from the source,
  // as required, e.g., for monitors.
  if (d.coords().contains(Dim("sample-position")) ||
      d.attrs().contains("sample-position"))
    return l1(d) + l2(d);
  else
    return norm(position(d) - source_position(d));
}

template <class T> static Variable l1(const T &d) {
  return norm(sample_position(d) - source_position(d));
}

template <class T> static Variable l2(const T &d) {
  // Use transform to avoid temporaries. For certain unit conversions this can
  // cause a speedup >50%. Short version would be:
  //   return norm(position(d) - sample_position(d));
  return transform<core::pair_self_t<Eigen::Vector3d>>(
      position(d), sample_position(d),
      overloaded{
          [](const auto &x, const auto &y) { return (x - y).norm(); },
          [](const units::Unit &x, const units::Unit &y) { return x - y; }});
}

template <class T> static Variable scattering_angle(const T &d) {
  return 0.5 * units::one * two_theta(d);
}

template <class T> static Variable two_theta(const T &d) {
  auto beam = sample_position(d) - source_position(d);
  const auto l1 = norm(beam);
  beam /= l1;
  auto scattered = position(d) - sample_position(d);
  const auto l2 = norm(scattered);
  scattered /= l2;

  return acos(dot(beam, scattered));
}
} // namespace beamline_impl

variable::VariableConstView position(const dataset::DatasetConstView &d) {
  return beamline_impl::position(d);
}
variable::VariableConstView
source_position(const dataset::DatasetConstView &d) {
  return beamline_impl::source_position(d);
}
variable::VariableConstView
sample_position(const dataset::DatasetConstView &d) {
  return beamline_impl::sample_position(d);
}
variable::VariableView position(const dataset::DatasetView &d) {
  return beamline_impl::position(d);
}
variable::VariableView source_position(const dataset::DatasetView &d) {
  return beamline_impl::source_position(d);
}
variable::VariableView sample_position(const dataset::DatasetView &d) {
  return beamline_impl::sample_position(d);
}
variable::Variable flight_path_length(const dataset::DatasetConstView &d) {
  return beamline_impl::flight_path_length(d);
}
variable::Variable l1(const dataset::DatasetConstView &d) {
  return beamline_impl::l1(d);
}
variable::Variable l2(const dataset::DatasetConstView &d) {
  return beamline_impl::l2(d);
}
variable::Variable scattering_angle(const dataset::DatasetConstView &d) {
  return beamline_impl::scattering_angle(d);
}
variable::Variable two_theta(const dataset::DatasetConstView &d) {
  return beamline_impl::two_theta(d);
}

variable::VariableConstView position(const dataset::DataArrayConstView &d) {
  return beamline_impl::position(d);
}
variable::VariableConstView
source_position(const dataset::DataArrayConstView &d) {
  return beamline_impl::source_position(d);
}
variable::VariableConstView
sample_position(const dataset::DataArrayConstView &d) {
  return beamline_impl::sample_position(d);
}
variable::VariableView position(const dataset::DataArrayView &d) {
  return beamline_impl::position(d);
}
variable::VariableView source_position(const dataset::DataArrayView &d) {
  return beamline_impl::source_position(d);
}
variable::VariableView sample_position(const dataset::DataArrayView &d) {
  return beamline_impl::sample_position(d);
}
variable::Variable flight_path_length(const dataset::DataArrayConstView &d) {
  return beamline_impl::flight_path_length(d);
}
variable::Variable l1(const dataset::DataArrayConstView &d) {
  return beamline_impl::l1(d);
}
variable::Variable l2(const dataset::DataArrayConstView &d) {
  return beamline_impl::l2(d);
}
variable::Variable scattering_angle(const dataset::DataArrayConstView &d) {
  return beamline_impl::scattering_angle(d);
}
variable::Variable two_theta(const dataset::DataArrayConstView &d) {
  return beamline_impl::two_theta(d);
}

} // namespace scipp::neutron
