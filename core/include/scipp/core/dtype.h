// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DTYPE_H
#define DTYPE_H

#include <Eigen/Dense>
#include <boost/container/small_vector.hpp>

#include "scipp/core/bool.h"

namespace pybind11 {
class object;
}

namespace scipp::core {

class DataArray;
class Dataset;

template <class T>
using sparse_container = boost::container::small_vector<T, 8>;

template <class T> struct is_sparse : std::false_type {};
template <class T> struct is_sparse<sparse_container<T>> : std::true_type {};
template <class T> struct is_sparse<sparse_container<T> &> : std::true_type {};
template <class T>
struct is_sparse<const sparse_container<T> &> : std::true_type {};
template <class T> inline constexpr bool is_sparse_v = is_sparse<T>::value;

enum class DType {
  Double,
  Float,
  Int32,
  Int64,
  String,
  Bool,
  SparseDouble,
  SparseFloat,
  SparseInt64,
  SparseInt32,
  DataArray,
  Dataset,
  EigenVector3d,
  PyObject,
  Unknown
};

template <class T> constexpr DType dtype = DType::Unknown;
template <> constexpr DType dtype<double> = DType::Double;
template <> constexpr DType dtype<float> = DType::Float;
template <> constexpr DType dtype<int32_t> = DType::Int32;
template <> constexpr DType dtype<int64_t> = DType::Int64;
template <> constexpr DType dtype<std::string> = DType::String;
template <> constexpr DType dtype<bool> = DType::Bool;
template <> constexpr DType dtype<Bool> = DType::Bool;
template <>
constexpr DType dtype<sparse_container<double>> = DType::SparseDouble;
template <> constexpr DType dtype<sparse_container<float>> = DType::SparseFloat;
template <>
constexpr DType dtype<sparse_container<int64_t>> = DType::SparseInt64;
template <>
constexpr DType dtype<sparse_container<int32_t>> = DType::SparseInt32;
template <> constexpr DType dtype<DataArray> = DType::DataArray;
template <> constexpr DType dtype<Dataset> = DType::Dataset;
template <> constexpr DType dtype<Eigen::Vector3d> = DType::EigenVector3d;
template <> constexpr DType dtype<pybind11::object> = DType::PyObject;

bool isInt(DType tp);
bool isFloatingPoint(DType tp);
bool isBool(DType tp);

} // namespace scipp::core

#endif // DTYPE_H
