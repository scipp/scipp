// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <set>

#include "scipp/core/element/arg_list.h"

#include "scipp/variable/event.h"
#include "scipp/variable/transform.h"
#include "scipp/variable/util.h"

#include "scipp/dataset/dataset.h"
#include "scipp/dataset/dataset_util.h"

#include "scipp/neutron/constants.h"
#include "scipp/neutron/conversions.h"
#include "scipp/neutron/convert.h"

using namespace scipp::variable;
using namespace scipp::dataset;

namespace scipp::neutron {

template <class T, class Op>
T convert_generic(T &&d, const Dim from, const Dim to,
                  const ConvertRealign realign, Op op,
                  const VariableConstView &arg) {
  using core::element::arg_list;
  const auto op_ = overloaded{arg_list<double, std::tuple<float, double>>, op};
  const auto items = iter(d);
  const bool any_aligned =
      std::any_of(items.begin(), items.end(),
                  [](const auto &item) { return item.hasData(); });
  // 1. Transform coordinate
  if (d.coords().contains(from)) {
    const auto coord = d.coords()[from];
    if (realign == ConvertRealign::None || any_aligned) {
      // Cannot realign if any item has aligned (histogrammed) data
      if (!coord.dims().contains(arg.dims()))
        d.coords().set(from, broadcast(coord, arg.dims()));
      transform_in_place(coord, arg, op_);
    } else {
      // Unit conversion may swap what min and max are, so we treat them jointly
      // as extrema and extract min and max at the end.
      auto extrema = concatenate(broadcast(min(coord, from), arg.dims()),
                                 broadcast(max(coord, from), arg.dims()), from);
      transform_in_place(extrema, arg, op_);
      d.coords().set(
          from, linspace(min(extrema), max(extrema), from, coord.dims()[from]));
    }
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
                             const ConvertRealign realign,
                             const Variable &factor) {
  return convert_generic(
      std::forward<T>(d), from, to, realign,
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

template <class T>
T convert_impl(T d, const Dim from, const Dim to,
               const ConvertRealign realign) {
  for (const auto &item : iter(d))
    if (item.hasData())
      core::expect::notCountDensity(item.unit());
  // This will need to be cleanup up in the future, but it is unclear how to do
  // so in a future-proof way. Some sort of double-dynamic dispatch based on
  // `from` and `to` will likely be required (with conversions helpers created
  // by a dynamic factory based on `Dim`). Conceptually we are dealing with a
  // bidirectional graph, and we would like to be able to find the shortest
  // paths between any two points, without defining all-to-all connections.
  // Approaches based on, e.g., a map of conversions and constants is also
  // tricky, since in particular the conversions are generic lambdas (passable
  // to `transform`) and are not readily stored as function pointers or
  // std::function.
  if ((from == Dim::Tof) && (to == Dim::DSpacing))
    return convert_with_factor(std::move(d), from, to, realign,
                               constants::tof_to_dspacing(d));
  if ((from == Dim::DSpacing) && (to == Dim::Tof))
    return convert_with_factor(std::move(d), from, to, realign,
                               reciprocal(constants::tof_to_dspacing(d)));

  if ((from == Dim::Tof) && (to == Dim::Wavelength))
    return convert_with_factor(std::move(d), from, to, realign,
                               constants::tof_to_wavelength(d));
  if ((from == Dim::Wavelength) && (to == Dim::Tof))
    return convert_with_factor(std::move(d), from, to, realign,
                               reciprocal(constants::tof_to_wavelength(d)));

  if ((from == Dim::Tof) && (to == Dim::Energy))
    return convert_generic(std::move(d), from, to, realign,
                           conversions::tof_to_energy,
                           constants::tof_to_energy(d));
  if ((from == Dim::Energy) && (to == Dim::Tof))
    return convert_generic(std::move(d), from, to, realign,
                           conversions::energy_to_tof,
                           constants::tof_to_energy(d));

  // lambda <-> Q conversion is symmetric
  if (((from == Dim::Wavelength) && (to == Dim::Q)) ||
      ((from == Dim::Q) && (to == Dim::Wavelength)))
    return convert_generic(std::move(d), from, to, realign,
                           conversions::wavelength_to_q,
                           constants::wavelength_to_q(d));

  throw except::UnitError(
      "Conversion between requested dimensions not implemented yet.");
}

namespace {
template <class T>
T swap_tof_related_labels_and_attrs(T &&x, const Dim from, const Dim to) {
  const auto to_attr = [&](const auto field) {
    if (!x.coords().contains(Dim(field)))
      return;
    if constexpr (std::is_same_v<std::decay_t<T>, Dataset>)
      for (const auto &item : iter(x))
        // TODO This is an unfortunate duplication of attributes. It is
        // (currently?) required due to a limitation of handling attributes of
        // Dataset and its items *independently* (no mapping of dataset
        // attributes into item attributes occurs, unlike for coords and
        // labels). If we did not also add the attributes to each of the items,
        // a subsequent unit conversion of an item on its own would not be
        // possible. It needs to be determined if there is a better way to
        // handle attributes so this can be avoided.
        item.attrs().set(field, x.coords()[Dim(field)]);
    x.attrs().set(field, x.coords()[Dim(field)]);
    x.coords().erase(Dim(field));
  };
  const auto to_coord = [&](const auto field) {
    if (!x.attrs().contains(field))
      return;
    x.coords().set(Dim(field), x.attrs()[field]);
    x.attrs().erase(field);
    if constexpr (std::is_same_v<std::decay_t<T>, Dataset>) {
      for (const auto &item : iter(x)) {
        core::expect::equals(x.coords()[Dim(field)], item.attrs()[field]);
        item.attrs().erase(field);
      }
    }
  };
  // Will be replaced by explicit flag
  bool scatter = x.coords().contains(Dim("sample-position"));
  if (scatter) {
    std::set<Dim> pos_invariant{Dim::DSpacing, Dim::Q};
    if (pos_invariant.count(to))
      to_attr("position");
    if (pos_invariant.count(from))
      to_coord("position");
  } else {
    if (to == Dim::Tof)
      to_coord("position");
    else
      to_attr("position");
  }
  return std::move(x);
}
} // namespace

DataArray convert(DataArray d, const Dim from, const Dim to,
                  const ConvertRealign realign) {
  return swap_tof_related_labels_and_attrs(
      convert_impl(std::move(d), from, to, realign), from, to);
}

DataArray convert(const DataArrayConstView &d, const Dim from, const Dim to,
                  const ConvertRealign realign) {
  return convert(DataArray(d), from, to, realign);
}

Dataset convert(Dataset d, const Dim from, const Dim to,
                const ConvertRealign realign) {
  return swap_tof_related_labels_and_attrs(
      convert_impl(std::move(d), from, to, realign), from, to);
}

Dataset convert(const DatasetConstView &d, const Dim from, const Dim to,
                const ConvertRealign realign) {
  return convert(Dataset(d), from, to, realign);
}

} // namespace scipp::neutron
