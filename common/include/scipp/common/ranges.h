// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <iterator>

namespace scipp::ranges {

template <class T> struct reverse_view { T &obj; };
template <class T> auto begin(reverse_view<T> w) { return std::rbegin(w.obj); }
template <class T> auto end(reverse_view<T> w) { return std::rend(w.obj); }

} // namespace scipp::ranges

namespace scipp::views {

template <class T> ranges::reverse_view<T> reverse(T &&obj) { return {obj}; }

} // namespace scipp::views
