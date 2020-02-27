// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_VIEW_FORWARD_H
#define SCIPP_CORE_VIEW_FORWARD_H

#include "scipp/core/axis_forward.h"
#include "scipp/core/dataset_access.h"
#include "scipp/units/dim.h"

namespace scipp::core {

class Variable;
class UnalignedAccess;

namespace ViewId {
class Attrs;
class Coords;
class Labels;
class Masks;
class Unaligned;
} // namespace ViewId
template <class Id, class Key, class Value> class ConstView;
template <class Base, class Access> class MutableView;

/// View for accessing coordinates of const Dataset and DataArrayConstView.
using CoordsConstView = ConstView<ViewId::Coords, Dim, DatasetAxis>;
/// View for accessing coordinates of Dataset and DataArrayView.
using CoordsView = MutableView<CoordsConstView, CoordAccess>;
/// View for accessing attributes of const Dataset and DataArrayConstView.
using AttrsConstView = ConstView<ViewId::Attrs, std::string, Variable>;
/// View for accessing attributes of Dataset and DataArrayView.
using AttrsView = MutableView<AttrsConstView, AttrAccess>;
/// View for accessing masks of const Dataset and DataArrayConstView
using MasksConstView = ConstView<ViewId::Masks, std::string, Variable>;
/// View for accessing masks of Dataset and DataArrayView
using MasksView = MutableView<MasksConstView, MaskAccess>;
/// View for accessing unaligned components of const DatasetAxis
using UnalignedConstView = ConstView<ViewId::Unaligned, std::string, Variable>;
/// View for accessing unaligned components of DatasetAxis
using UnalignedView = MutableView<UnalignedConstView, UnalignedAccess>;

} // namespace scipp::core

#endif // SCIPP_CORE_VIEW_FORWARD_H
