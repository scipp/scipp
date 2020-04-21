// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Dimitar Tasev
#pragma once

#include "scipp/dataset/dataset_access.h"
#include "scipp/units/dim.h"

namespace scipp::variable {
class Variable;
}

namespace scipp::dataset {

namespace ViewId {
class Attrs;
class Coords;
class Labels;
class Masks;
} // namespace ViewId
template <class Id, class Key, class Value> class ConstView;
template <class Base, class Access> class MutableView;

/// View for accessing coordinates of const DataArray and Dataset.
using CoordsConstView = ConstView<ViewId::Coords, Dim, variable::Variable>;
/// View for accessing coordinates of DataArray and Dataset.
using CoordsView = MutableView<CoordsConstView, CoordAccess>;
/// View for accessing attributes of const Dataset and DataArrayConstView.
using AttrsConstView =
    ConstView<ViewId::Attrs, std::string, variable::Variable>;
/// View for accessing attributes of Dataset and DataArrayView.
using AttrsView = MutableView<AttrsConstView, AttrAccess>;
/// View for accessing masks of const Dataset and DataArrayConstView
using MasksConstView =
    ConstView<ViewId::Masks, std::string, variable::Variable>;
/// View for accessing masks of Dataset and DataArrayView
using MasksView = MutableView<MasksConstView, MaskAccess>;

} // namespace scipp::dataset
