/// @file
/// SPDX-License-Identifier: GPL-3.0-or-later
/// @author Simon Heybrock
/// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
/// National Laboratory, and European Spallation Source ERIC.
#ifndef INDEX_H
#define INDEX_H

namespace scipp {
/// Type to use for all container/array sizes and indices.
//
// As recommended by the C++ core guidelines, this is signed, i.e., not size_t.
using index = std::ptrdiff_t;

/// Return the size of a container as a signed index type.
//
// The purpose of this is to improve interoperability with std containers,
// where, e.g., std::vector::size return size_t. Use of this free function
// reduces the need for manual casting, which would otherwise be required to
// avoid compiler warnings.
template <class T> index size(const T &container) {
  return static_cast<index>(container.size());
}
} // namespace scipp

#endif // INDEX_H
