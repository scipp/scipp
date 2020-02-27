// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_AXIS_FORWARD_H
#define SCIPP_CORE_AXIS_FORWARD_H

#include <string>
#include <unordered_map>

namespace scipp::core {

class Variable;

namespace AxisId {
class DataArray;
class Dataset;
} // namespace AxisId

namespace axis {
using dataset_unaligned_type = std::unordered_map<std::string, Variable>;
}

template <class T, class U> class Axis;
using DatasetAxis = Axis<AxisId::Dataset, axis::dataset_unaligned_type>;

} // namespace scipp::core

#endif // SCIPP_CORE_AXIS_FORWARD_H
