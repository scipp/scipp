// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DTYPE_H
#define SCIPP_CORE_DTYPE_H

#include <Eigen/Dense>
#include <boost/container/small_vector.hpp>

namespace scipp::python {
class PyObject;
}

namespace scipp::dataset {
class DataArray;
class Dataset;
} // namespace scipp::dataset

namespace scipp::core {

template <class T>
using sparse_container = boost::container::small_vector<T, 8>;
template <class T> using event_list = sparse_container<T>;

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
  SparseBool,
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
template <>
constexpr DType dtype<sparse_container<double>> = DType::SparseDouble;
template <> constexpr DType dtype<sparse_container<float>> = DType::SparseFloat;
template <>
constexpr DType dtype<sparse_container<int64_t>> = DType::SparseInt64;
template <>
constexpr DType dtype<sparse_container<int32_t>> = DType::SparseInt32;
template <> constexpr DType dtype<sparse_container<bool>> = DType::SparseBool;
template <> constexpr DType dtype<dataset::DataArray> = DType::DataArray;
template <> constexpr DType dtype<dataset::Dataset> = DType::Dataset;
template <> constexpr DType dtype<Eigen::Vector3d> = DType::EigenVector3d;
template <> constexpr DType dtype<scipp::python::PyObject> = DType::PyObject;

bool isInt(DType tp);

DType event_dtype(const DType type);

namespace detail {
template <class T> struct element_type { using type = T; };
template <class T> struct element_type<sparse_container<T>> { using type = T; };
template <class T> struct element_type<const sparse_container<T>> {
  using type = T;
};
template <class T> using element_type_t = typename element_type<T>::type;
} // namespace detail

} // namespace scipp::core

#endif // SCIPP_CORE_DTYPE_H
