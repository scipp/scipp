// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
#include <ostream>

#include "scipp/core/dtype.h"
#include "scipp/core/eigen.h"
#include "scipp/core/spatial_transforms.h"
#include "scipp/core/string.h"

namespace scipp::core {

bool is_int(DType tp) { return tp == dtype<int32_t> || tp == dtype<int64_t>; }

bool is_float(DType tp) { return tp == dtype<float> || tp == dtype<double>; }

bool is_fundamental(DType tp) {
  return is_int(tp) || is_float(tp) || tp == dtype<bool>;
}

bool is_total_orderable(DType tp) {
  return is_fundamental(tp) || tp == dtype<time_point>;
}

namespace {
template <class... Ts> bool is_span_impl(DType tp) {
  return (((tp == dtype<scipp::span<Ts>>) ||
           (tp == dtype<scipp::span<const Ts>>)) ||
          ...);
}
} // namespace

bool is_span(DType tp) {
  return is_span_impl<double, float, int64_t, int32_t, bool, time_point>(tp);
}

bool is_structured(DType tp) {
  return tp == dtype<Eigen::Vector3d> || tp == dtype<Eigen::Matrix3d> || tp == dtype<scipp::core::Quaternion> || tp == dtype<Eigen::Affine3d> || tp == dtype<scipp::core::Translation>;
}

std::ostream &operator<<(std::ostream &os, const DType &dtype) {
  return os << to_string(dtype);
}

auto register_dtype_name_void(
    (core::dtypeNameRegistry().emplace(dtype<void>, "<none>"), 0));

} // namespace scipp::core
