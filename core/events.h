// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef EVENTS_H
#define EVENTS_H

#include <initializer_list>
#include <vector>

#include "dimension.h"

namespace scipp::core {

class Dataset;

namespace events {
void sortByTof(Dataset &dataset);
} // namespace events
} // namespace scipp::core

#endif // EVENTS_H
