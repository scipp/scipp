// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/units/dim.h"

namespace scipp::units {

std::unordered_map<std::string, DimId> Dim::builtin_ids{
    {"detector", Dim::Detector},
    {"d-spacing", Dim::DSpacing},
    {"E", Dim::Energy},
    {"Delta-E", Dim::EnergyTransfer},
    {"group", Dim::Group},
    {"<invalid>", Dim::Invalid},
    {"position", Dim::Position},
    {"Q", Dim::Q},
    {"Q_x", Dim::Qx},
    {"Q_y", Dim::Qy},
    {"Q_z", Dim::Qz},
    {"Q^2", Dim::QSquared},
    {"row", Dim::Row},
    {"scattering-angle", Dim::ScatteringAngle},
    {"spectrum", Dim::Spectrum},
    {"temperature", Dim::Temperature},
    {"time", Dim::Time},
    {"tof", Dim::Tof},
    {"wavelength", Dim::Wavelength},
    {"x", Dim::X},
    {"y", Dim::Y},
    {"z", Dim::Z}};

std::unordered_map<std::string, DimId> Dim::custom_ids;
std::mutex Dim::mutex;

std::string to_string(const Dim dim) { return dim.name(); }

} // namespace scipp::units
