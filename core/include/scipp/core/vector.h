// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef VECTOR_H
#define VECTOR_H

#include "scipp/core/element_array.h"

namespace scipp::core {
template <class T> using Vector = detail::element_array<T>;
}

#endif // VECTOR_H
