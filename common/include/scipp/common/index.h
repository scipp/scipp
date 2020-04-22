// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <cstdint>

namespace scipp {
/// Type to use for all container/array sizes and indices.
///
/// As recommended by the C++ core guidelines, this is signed, i.e., not size_t.
using index = int64_t;

/// Return the size of a container as a signed index type.
///
/// The purpose of this is to improve interoperability with std containers,
/// where, e.g., std::vector::size return size_t. Use of this free function
/// reduces the need for manual casting, which would otherwise be required to
/// avoid compiler warnings.
template <class T> index size(const T &container) {
  return static_cast<index>(container.size());
}
} // namespace scipp
