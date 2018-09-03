#ifndef VECTOR_H
#define VECTOR_H

#include <vector>

#include "aligned_allocator.h"

template <class T>
using Vector = std::vector<T, AlignedAllocator<T, Alignment::AVX>>;

#endif // VECTOR_H
