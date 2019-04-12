// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef CONVERT_H
#define CONVERT_H

#include <vector>

#include "dimension.h"

namespace scipp::core {

class Dataset;

Dataset convert(const Dataset &d, const Dim from, const Dim to);
Dataset convert(const Dataset &d, const std::vector<Dim> &from,
                const Dataset &toCoords);

} // namespace scipp::core

#endif // CONVERT_H
