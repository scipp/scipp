// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include "scipp-neutron_export.h"
#include "scipp/dataset/dataset.h"
#include "scipp/units/unit.h"

namespace scipp::neutron {

SCIPP_NEUTRON_EXPORT dataset::DataArray convert(dataset::DataArray d,
                                                const Dim from, const Dim to);
SCIPP_NEUTRON_EXPORT dataset::DataArray
convert(const dataset::DataArrayConstView &d, const Dim from, const Dim to);
SCIPP_NEUTRON_EXPORT dataset::Dataset convert(dataset::Dataset d,
                                              const Dim from, const Dim to);
SCIPP_NEUTRON_EXPORT dataset::Dataset
convert(const dataset::DatasetConstView &d, const Dim from, const Dim to);

class SCIPP_NEUTRON_EXPORT TofBeamline {
public:
  TofBeamline(const std::vector<std::string> &path,
              const std::optional<std::string> &scatter);

  Variable theta(const dataset::DataArrayConstView &d) const;
  Variable two_theta(const dataset::DataArrayConstView &d) const;
  Variable l1(const dataset::DataArrayConstView &d) const;
  Variable l2(const dataset::DataArrayConstView &d) const;
  Variable flight_path_length(const dataset::DataArrayConstView &d) const;

  Variable theta(const dataset::DatasetConstView &d) const;
  Variable two_theta(const dataset::DatasetConstView &d) const;
  Variable l1(const dataset::DatasetConstView &d) const;
  Variable l2(const dataset::DatasetConstView &d) const;
  Variable flight_path_length(const dataset::DatasetConstView &d) const;

  dataset::DataArray convert(dataset::DataArray d, const Dim from, const Dim to,
                             const bool scatter) const;
  dataset::Dataset convert(dataset::Dataset d, const Dim from, const Dim to,
                           const bool scatter) const;

private:
  template <class T>
  T do_convert(T d, const Dim from, const Dim to, const bool scatter) const;
  std::vector<std::string> m_path;
  std::optional<std::string> m_scatter;
};

class SCIPP_NEUTRON_EXPORT ElasticScattering {
public:
  ElasticScattering(const std::string &theta);

  dataset::DataArray convert(dataset::DataArray d, const Dim from,
                             const Dim to) const;
  dataset::Dataset convert(dataset::Dataset d, const Dim from,
                           const Dim to) const;

private:
  template <class T> T do_convert(T d, const Dim from, const Dim to) const;
  std::string m_theta;
};
} // namespace scipp::neutron
