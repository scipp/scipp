/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
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
