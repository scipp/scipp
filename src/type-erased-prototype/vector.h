/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef VECTOR_H
#define VECTOR_H

#include <vector>

#include "aligned_allocator.h"

template <class T>
using Vector = std::vector<T, AlignedAllocator<T, Alignment::AVX>>;

#endif // VECTOR_H
