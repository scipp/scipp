// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef VECTOR_H
#define VECTOR_H

#include <vector>

#include "scipp/core/aligned_allocator.h"

namespace scipp::core {

template <class T>
using Vector = std::vector<T, AlignedAllocator<T, Alignment::AVX>>;
}

#endif // VECTOR_H
