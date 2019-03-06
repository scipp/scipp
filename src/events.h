/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef EVENTS_H
#define EVENTS_H

#include <initializer_list>
#include <vector>

#include "dimension.h"

class Dataset;

namespace events {
void sortByTof(Dataset &dataset);
} // namespace events

#endif // EVENTS_H
