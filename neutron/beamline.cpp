// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock

#include "scipp/neutron/beamline.h"
#include "scipp/core/dataset.h"
#include "scipp/core/transform.h"

using namespace scipp::core;

namespace scipp::neutron {

auto component_positions(const Dataset &d) {
  return d.labels()["component_info"].values<Dataset>()[0]["position"].data();
}

VariableConstProxy position(const core::Dataset &d) {
  if (d.coords().contains(Dim::Position))
    return d.coords()[Dim::Position];
  else
    return d.labels()["position"];
}

Variable source_position(const Dataset &d) {
  // TODO Need a better mechanism to identify source and sample.
  return Variable(component_positions(d).slice({Dim::Row, 0}));
}

Variable sample_position(const Dataset &d) {
  return Variable(component_positions(d).slice({Dim::Row, 1}));
}

Variable l1(const Dataset &d) {
  return norm(sample_position(d) - source_position(d));
}

Variable l2(const Dataset &d) {
  // Use transform to avoid temporaries. For certain unit conversions this can
  // cause a speedup >50%. Short version would be:
  //   return norm(position(d) - sample_position(d));
  return transform<pair_self_t<Eigen::Vector3d>>(
      position(d), sample_position(d),
      overloaded{
          [](const auto &x, const auto &y) { return (x - y).norm(); },
          [](const units::Unit &x, const units::Unit &y) { return x - y; }});
}

Variable scattering_angle(const Dataset &d) { return 0.5 * two_theta(d); }

Variable two_theta(const Dataset &d) {
  auto beam = sample_position(d) - source_position(d);
  const auto l1 = norm(beam);
  beam /= l1;
  auto scattered = position(d) - sample_position(d);
  const auto l2 = norm(scattered);
  scattered /= l2;

  return acos(dot(beam, scattered));
}

} // namespace scipp::neutron
