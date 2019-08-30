// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_COMMON_OVERLOADED_H
#define SCIPP_COMMON_OVERLOADED_H

namespace scipp {
/// Helper for creating overloaded operators from multiple lambdas.
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;
} // namespace scipp

#endif // SCIPP_COMMON_OVERLOADED_H
