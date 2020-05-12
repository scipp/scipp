// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/element/arg_list.h"

#include "scipp/variable/event.h"
#include "scipp/variable/transform.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/dataset_util.h"

#include "scipp/neutron/constants.h"
#include "scipp/neutron/conversions.h"
#include "scipp/neutron/convert.h"

using namespace scipp::variable;
using namespace scipp::dataset;

namespace scipp::neutron {

template <class T, class Op>
T convert_generic(T &&d, const Dim from, const Dim to, Op op,
                  const VariableConstView &arg) {
  using core::element::arg_list;
  const auto op_ = overloaded{
      arg_list<std::pair<double, double>, std::pair<float, double>>, op};
  // 1. Transform coordinate
  if (d.coords().contains(from)) {
    if (!d.coords()[from].dims().contains(arg.dims()))
      d.coords().set(from, broadcast(d.coords()[from], arg.dims()));
    transform_in_place(d.coords()[from], arg, op_);
  }
  // 2. Transform realigned items
  for (const auto &item : iter(d))
    if (item.unaligned() && contains_events(item.unaligned()))
      transform_in_place(item.unaligned().coords()[from], arg, op_);
  d.rename(from, to);
  return std::move(d);
}

template <class T>
static T convert_with_factor(T &&d, const Dim from, const Dim to,
                             const Variable &factor) {
  return convert_generic(
      std::forward<T>(d), from, to,
      [](auto &coord, const auto &c) { coord *= c; }, factor);
}

/*
Dataset tofToDeltaE(const Dataset &d) {
  // There are two cases, direct inelastic and indirect inelastic. We can
  // distinguish them by the content of d.
  if (d.contains(Coord::Ei) && d.contains(Coord::Ef))
    throw std::runtime_error("Dataset contains Coord::Ei as well as Coord::Ef, "
                             "cannot have both for inelastic scattering.");

  // 1. Compute conversion factors
  const auto &compPos = d.get(Coord::ComponentInfo)[0](Coord::Position);
  const auto &sourcePos = compPos(Dim::Component, 0);
  const auto &samplePos = compPos(Dim::Component, 1);
  auto l1_square = norm(sourcePos - samplePos);
  l1_square *= l1_square;
  l1_square *= tofToEnergyPhysicalConstants;
  const auto specPos = getSpecPos(d);
  auto l2_square = norm(specPos - samplePos);
  l2_square *= l2_square;
  l2_square *= tofToEnergyPhysicalConstants;

  auto tofShift = makeVariable<double>({});
  auto scale = makeVariable<double>({});

  if (d.contains(Coord::Ei)) {
    // Direct-inelastic.
    // This is how we support multi-Ei data!
    tofShift = sqrt(l1_square / d(Coord::Ei));
    scale = std::move(l2_square);
  } else if (d.contains(Coord::Ef)) {
    // Indirect-inelastic.
    // Ef can be different for every spectrum.
    tofShift = sqrt(std::move(l2_square) / d(Coord::Ef));
    scale = std::move(l1_square);
  } else {
    throw std::runtime_error("Dataset contains neither Coord::Ei nor "
                             "Coord::Ef, this does not look like "
                             "inelastic-scattering data.");
  }

  // 2. Transform variables
  Dataset converted;
  for (const auto & [ name, tag, var ] : d) {
    auto varDims = var.dimensions();
    if (varDims.contains(Dim::Tof))
      varDims.relabel(varDims.index(Dim::Tof), Dim::DeltaE);
    if (tag == Coord::Tof) {
      Variable inv_tof = 1.0 / (var.reshape(varDims) - tofShift);
      Variable E = inv_tof * inv_tof * scale;
      if (d.contains(Coord::Ei)) {
        converted.insert(Coord::DeltaE, -(std::move(E) - d(Coord::Ei)));
      } else {
        converted.insert(Coord::DeltaE, std::move(E) - d(Coord::Ef));
      }
    } else if (tag == Data::Events) {
      throw std::runtime_error(
          "TODO Converting units of event data not implemented yet.");
    } else {
      if (counts::isDensity(var))
        throw std::runtime_error("TODO Converting units of count-density data "
                                 "not implemented yet for this case.");
      converted.insert(tag, name, var.reshape(varDims));
    }
  }

  // TODO Do we always require reversing for inelastic?
  // TODO Is is debatable whether this should revert automatically... probably
  // not, but we need to put a check in place for `rebin` to fail if the axis is
  // reversed.
  return reverse(converted, Dim::DeltaE);
}
*/

template <class T> T convert_impl(T d, const Dim from, const Dim to) {
  for (const auto &item : iter(d))
    if (item.hasData())
      core::expect::notCountDensity(item.unit());
  if ((from == Dim::Tof) && (to == Dim::DSpacing))
    return convert_with_factor(std::move(d), from, to,
                               constants::tof_to_dspacing(d));
  if ((from == Dim::DSpacing) && (to == Dim::Tof))
    return convert_with_factor(std::move(d), from, to,
                               reciprocal(constants::tof_to_dspacing(d)));

  if ((from == Dim::Tof) && (to == Dim::Wavelength))
    return convert_with_factor(std::move(d), from, to,
                               constants::tof_to_wavelength(d));
  if ((from == Dim::Wavelength) && (to == Dim::Tof))
    return convert_with_factor(std::move(d), from, to,
                               reciprocal(constants::tof_to_wavelength(d)));

  if ((from == Dim::Tof) && (to == Dim::Energy))
    return convert_generic(std::move(d), from, to, conversions::tof_to_energy,
                           constants::tof_to_energy(d));
  if ((from == Dim::Energy) && (to == Dim::Tof))
    return convert_generic(std::move(d), from, to, conversions::energy_to_tof,
                           constants::tof_to_energy(d));

  // lambda <-> Q conversion is symmetric
  if (((from == Dim::Wavelength) && (to == Dim::Q)) ||
      ((from == Dim::Q) && (to == Dim::Wavelength)))
    return convert_generic(std::move(d), from, to, conversions::wavelength_to_q,
                           constants::wavelength_to_q(d));

  throw except::UnitError(
      "Conversion between requested dimensions not implemented yet.");
}

namespace {
template <class T>
T swap_tof_related_labels_and_attrs(T &&x, const Dim from, const Dim to) {
  auto fields = {"position", "source_position", "sample_position"};
  // TODO Add `extract` methods to do this in one step and avoid copies?
  if (from == Dim::Tof) {
    for (const auto &field : fields) {
      if (x.coords().contains(Dim(field))) {
        // TODO This is an unfortunate duplication of attributes. It is
        // (currently?) required due to a limitation of handling attributes of
        // Dataset and its items *independently* (no mapping of dataset
        // attributes into item attributes occurs, unlike for coords and
        // labels). If we did not also add the attributes to each of the items,
        // a subsequent unit conversion of an item on its own would not be
        // possible. It needs to be determined if there is a better way to
        // handle attributes so this can be avoided.
        if constexpr (std::is_same_v<std::decay_t<T>, Dataset>)
          for (const auto &item : iter(x))
            item.attrs().set(field, x.coords()[Dim(field)]);
        x.attrs().set(field, x.coords()[Dim(field)]);
        x.coords().erase(Dim(field));
      }
    }
  }
  if (to == Dim::Tof) {
    for (const auto &field : fields) {
      if (x.attrs().contains(field)) {
        x.coords().set(Dim(field), x.attrs()[field]);
        x.attrs().erase(field);
        if constexpr (std::is_same_v<std::decay_t<T>, Dataset>) {
          for (const auto &item : iter(x)) {
            core::expect::equals(x.coords()[Dim(field)], item.attrs()[field]);
            item.attrs().erase(field);
          }
        }
      }
    }
  }
  return std::move(x);
}
} // namespace

DataArray convert(DataArray d, const Dim from, const Dim to) {
  return swap_tof_related_labels_and_attrs(convert_impl(std::move(d), from, to),
                                           from, to);
}

DataArray convert(const DataArrayConstView &d, const Dim from, const Dim to) {
  return convert(DataArray(d), from, to);
}

Dataset convert(Dataset d, const Dim from, const Dim to) {
  return swap_tof_related_labels_and_attrs(convert_impl(std::move(d), from, to),
                                           from, to);
}

Dataset convert(const DatasetConstView &d, const Dim from, const Dim to) {
  return convert(Dataset(d), from, to);
}

} // namespace scipp::neutron
