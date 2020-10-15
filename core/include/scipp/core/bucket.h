// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once
#include <utility>

#include "scipp-core_export.h"
#include "scipp/common/index.h"

namespace scipp::core {

struct bucket_base {
  using range_type = std::pair<scipp::index, scipp::index>;
};
template <class T> struct bucket : bucket_base {
  using buffer_type = T;
  using element_type = typename T::view_type;
  using const_element_type = typename T::const_view_type;
};

} // namespace scipp::core

namespace scipp {
using core::bucket;
}
