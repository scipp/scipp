/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef COUNTS_H
#define COUNTS_H

#include "dimension.h"

class Dataset;

namespace counts {
Dataset toDensity(Dataset d, const Dim dim);
Dataset fromDensity(Dataset d, const Dim dim);
} // namespace counts

#endif // COUNTS_H
