// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DTYPE_H
#define DTYPE_H

#include <Eigen/Dense>
#include <boost/container/small_vector.hpp>

#include "bool.h"

namespace scipp::core {

class Dataset;

enum class DType {
  Double,
  Float,
  Int32,
  Int64,
  String,
  Char,
  Bool,
  SmallVectorDouble8,
  Dataset,
  EigenVector3d,
  Unknown
};
template <class T> constexpr DType dtype = DType::Unknown;
template <> constexpr DType dtype<double> = DType::Double;
template <> constexpr DType dtype<float> = DType::Float;
template <> constexpr DType dtype<int32_t> = DType::Int32;
template <> constexpr DType dtype<int64_t> = DType::Int64;
template <> constexpr DType dtype<std::string> = DType::String;
template <> constexpr DType dtype<char> = DType::Char;
template <> constexpr DType dtype<bool> = DType::Bool;
template <> constexpr DType dtype<Bool> = DType::Bool;
template <>
constexpr DType dtype<boost::container::small_vector<double, 8>> =
    DType::SmallVectorDouble8;
template <> constexpr DType dtype<Dataset> = DType::Dataset;
template <> constexpr DType dtype<Eigen::Vector3d> = DType::EigenVector3d;

} // namespace scipp::core

#endif // DTYPE_H
