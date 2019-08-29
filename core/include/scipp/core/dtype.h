// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#ifndef DTYPE_H
#define DTYPE_H

#include <Eigen/Dense>
#include <boost/container/small_vector.hpp>
#include <typeinfo>

#include "scipp/core/bool.h"

namespace scipp::core {

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
template <> constexpr DType dtype<bool> = DType::Bool;
template <> constexpr DType dtype<Bool> = DType::Bool;
template <>
constexpr DType dtype<sparse_container<double>> = DType::SparseDouble;
template <> constexpr DType dtype<sparse_container<float>> = DType::SparseFloat;
template <>
constexpr DType dtype<sparse_container<int64_t>> = DType::SparseInt64;
template <>
constexpr DType dtype<sparse_container<int32_t>> = DType::SparseInt32;
template <> constexpr DType dtype<Dataset> = DType::Dataset;
template <> constexpr DType dtype<Eigen::Vector3d> = DType::EigenVector3d;

template <DType tp> struct TypeMap {};
template <> struct TypeMap<DType::Double> { using type = double; };
template <> struct TypeMap<DType::Float> { using type = float; };
template <> struct TypeMap<DType::Int32> { using type = int32_t; };
template <> struct TypeMap<DType::Int64> { using type = int64_t; };
template <> struct TypeMap<DType::String> { using type = std::string; };
template <> struct TypeMap<DType::Bool> { using type = Bool; };
template <> struct TypeMap<DType::SparseDouble> {
  using type = sparse_container<double>;
};
template <> struct TypeMap<DType::SparseFloat> {
  using type = sparse_container<float>;
};
template <> struct TypeMap<DType::SparseInt64> {
  using type = sparse_container<int64_t>;
};
template <> struct TypeMap<DType::SparseInt32> {
  using type = sparse_container<int32_t>;
};
template <> struct TypeMap<DType::Dataset> { using type = Dataset; };
template <> struct TypeMap<DType::EigenVector3d> {
  using type = Eigen::Vector3d;
};

template <class T1, class T2>
constexpr bool Enable =
    std::is_same_v<T1, T2> || std::is_trivially_constructible_v<T1, T2>;

template <class T1, class T2, bool = Enable<T1, T2>> struct castHelper {};

template <class T1, class T2> struct castHelper<T1, T2, false> {
  static T1 cast(const T2 &arg) {
    throw std::runtime_error(std::string("Can't cast: ") + typeid(arg).name() +
                             " and " + typeid(T1{}).name());
  }
};

template <class T1, class T2> struct castHelper<T1, T2, true> {
  static T1 cast(const T2 &arg) { return T1(arg); }
};

#define MAKE_VARIABLE_CASE(tp, x)                                              \
  case DType::tp:                                                              \
    return makeVariable<TypeMap<DType::tp>::type>(                             \
        castHelper<TypeMap<DType::tp>::type, decltype(x)>::cast(x));           \
    break;

#define MAKE_VARIABLE_DTYPED(tp, x)                                            \
  switch (tp) {                                                                \
    MAKE_VARIABLE_CASE(Double, x)                                              \
    MAKE_VARIABLE_CASE(Float, x)                                               \
    MAKE_VARIABLE_CASE(Int32, x)                                               \
    MAKE_VARIABLE_CASE(Int64, x)                                               \
    MAKE_VARIABLE_CASE(String, x)                                              \
    MAKE_VARIABLE_CASE(Bool, x)                                                \
    MAKE_VARIABLE_CASE(SparseDouble, x)                                        \
    MAKE_VARIABLE_CASE(SparseFloat, x)                                         \
    MAKE_VARIABLE_CASE(SparseInt64, x)                                         \
    MAKE_VARIABLE_CASE(SparseInt32, x)                                         \
    MAKE_VARIABLE_CASE(Dataset, x)                                             \
    MAKE_VARIABLE_CASE(EigenVector3d, x)                                       \
  case DType::Unknown:                                                         \
    throw std::logic_error("Can't convert to Unknown type");                   \
    return {};                                                                 \
  }

bool isInt(DType tp);
bool isFloatingPoint(DType tp);
bool isBool(DType tp);

} // namespace scipp::core

#endif // DTYPE_H
