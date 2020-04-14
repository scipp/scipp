// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef SCIPP_CORE_DTYPE_H
#define SCIPP_CORE_DTYPE_H

#include <Eigen/Dense>
#include <boost/container/small_vector.hpp>

#include "scipp/common/span.h"

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
  EigenQuaterniond,
  PyObject,
  SpanDouble,
  SpanFloat,
  SpanInt64,
  SpanInt32,
  SpanConstDouble,
  SpanConstFloat,
  SpanConstInt64,
  SpanConstInt32,
  Unknown
};

template <class T> inline constexpr DType dtype = DType::Unknown;
template <> inline constexpr DType dtype<double> = DType::Double;
template <> inline constexpr DType dtype<float> = DType::Float;
template <> inline constexpr DType dtype<int32_t> = DType::Int32;
template <> inline constexpr DType dtype<int64_t> = DType::Int64;
template <> inline constexpr DType dtype<std::string> = DType::String;
template <> inline constexpr DType dtype<bool> = DType::Bool;
template <>
inline constexpr DType dtype<sparse_container<double>> = DType::SparseDouble;
template <>
inline constexpr DType dtype<sparse_container<float>> = DType::SparseFloat;
template <>
inline constexpr DType dtype<sparse_container<int64_t>> = DType::SparseInt64;
template <>
inline constexpr DType dtype<sparse_container<int32_t>> = DType::SparseInt32;
template <>
inline constexpr DType dtype<sparse_container<bool>> = DType::SparseBool;
template <> inline constexpr DType dtype<span<double>> = DType::SpanDouble;
template <> inline constexpr DType dtype<span<float>> = DType::SpanFloat;
template <> inline constexpr DType dtype<span<int64_t>> = DType::SpanInt64;
template <> inline constexpr DType dtype<span<int32_t>> = DType::SpanInt32;
template <>
inline constexpr DType dtype<span<const double>> = DType::SpanConstDouble;
template <>
inline constexpr DType dtype<span<const float>> = DType::SpanConstFloat;
template <>
inline constexpr DType dtype<span<const int64_t>> = DType::SpanConstInt64;
template <>
inline constexpr DType dtype<span<const int32_t>> = DType::SpanConstInt32;
template <> inline constexpr DType dtype<dataset::DataArray> = DType::DataArray;
template <> inline constexpr DType dtype<dataset::Dataset> = DType::Dataset;
template <>
inline constexpr DType dtype<Eigen::Vector3d> = DType::EigenVector3d;
template <>
inline constexpr DType dtype<Eigen::Quaterniond> = DType::EigenQuaterniond;
template <>
inline constexpr DType dtype<scipp::python::PyObject> = DType::PyObject;

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

template <class T> constexpr bool canHaveVariances() noexcept {
  using U = std::remove_const_t<T>;
  return std::is_same_v<U, double> || std::is_same_v<U, float> ||
         std::is_same_v<U, sparse_container<double>> ||
         std::is_same_v<U, sparse_container<float>> ||
         std::is_same_v<U, span<const double>> ||
         std::is_same_v<U, span<const float>> ||
         std::is_same_v<U, span<double>> || std::is_same_v<U, span<float>>;
}

} // namespace scipp::core

#endif // SCIPP_CORE_DTYPE_H
