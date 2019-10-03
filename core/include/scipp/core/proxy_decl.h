// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Dimitar Tasev
#ifndef SCIPP_CORE_PROXY_DECL_H
#define SCIPP_CORE_PROXY_DECL_H
namespace scipp::core {

namespace ProxyId {
class Attrs;
class Coords;
class Labels;
class Masks;
} // namespace ProxyId
template <class Id, class Key> class ConstProxy;
template <class Base> class MutableProxy;

/// Proxy for accessing coordinates of const Dataset and DataConstProxy.
using CoordsConstProxy = ConstProxy<ProxyId::Coords, Dim>;
/// Proxy for accessing coordinates of Dataset and DataProxy.
using CoordsProxy = MutableProxy<CoordsConstProxy>;
/// Proxy for accessing labels of const Dataset and DataConstProxy.
using LabelsConstProxy = ConstProxy<ProxyId::Labels, std::string>;
/// Proxy for accessing labels of Dataset and DataProxy.
using LabelsProxy = MutableProxy<LabelsConstProxy>;
/// Proxy for accessing attributes of const Dataset and DataConstProxy.
using AttrsConstProxy = ConstProxy<ProxyId::Attrs, std::string>;
/// Proxy for accessing attributes of Dataset and DataProxy.
using AttrsProxy = MutableProxy<AttrsConstProxy>;
/// Proxy for accessing masks of const Dataset and DataConstProxy
using MasksConstProxy = ConstProxy<ProxyId::Masks, std::string>;
/// Proxy for accessing masks of Dataset and DataProxy
using MasksProxy = MutableProxy<MasksConstProxy>;

} // namespace scipp::core
#endif // SCIPP_CORE_PROXY_DECL_H