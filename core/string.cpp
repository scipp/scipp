// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/core/string.h"
#include "scipp/core/dimensions.h"
#include "scipp/core/slice.h"

namespace scipp::core {

std::ostream &operator<<(std::ostream &os, const Dimensions &dims) {
  return os << to_string(dims);
}

constexpr const char *tab = "    ";

std::string to_string(const Dimensions &dims) {
  if (dims.empty())
    return "{}";
  std::string s = "{{";
  for (int32_t i = 0; i < dims.shape().size(); ++i)
    s += to_string(dims.labels()[i]) + ", " + std::to_string(dims.shape()[i]) +
         "}, {";
  s.resize(s.size() - 3);
  s += "}";
  return s;
}

const std::string &to_string(const std::string &s) { return s; }
std::string_view to_string(const std::string_view s) { return s; }
std::string to_string(const char *s) { return std::string(s); }

std::string to_string(const bool b) { return b ? "True" : "False"; }

std::string to_string(const DType dtype) {
  return "todo";
  /*
  switch (dtype) {
  case DType::String:
    return "string";
  case DType::Bool:
    return "bool";
  case DType::DataArray:
    return "DataArray";
  case DType::Dataset:
    return "Dataset";
  case DType::Float:
    return "float32";
  case DType::Double:
    return "float64";
  case DType::Int32:
    return "int32";
  case DType::Int64:
    return "int64";
  case DType::SparseFloat:
    return "event_list_float32";
  case DType::SparseDouble:
    return "event_list_float64";
  case DType::SparseInt64:
    return "event_list_int64";
  case DType::SparseInt32:
    return "event_list_int32";
  case DType::EigenVector3d:
    return "vector_3_float64";
  case DType::EigenQuaterniond:
    return "quaternion_float64";
  case DType::PyObject:
    return "PyObject";
  case DType::Unknown:
    return "unknown";
  case DType::SpanFloat: // defined after Unknown so no Python binding created
    return "span_float32";
  case DType::SpanDouble:
    return "span_float64";
  case DType::SpanInt64:
    return "span_int64";
  case DType::SpanInt32:
    return "span_int32";
  case DType::SpanConstFloat:
    return "span_const_float32";
  case DType::SpanConstDouble:
    return "span_const_float64";
  case DType::SpanConstInt64:
    return "span_const_int64";
  case DType::SpanConstInt32:
    return "span_const_int32";
  default:
    return "unregistered dtype";
  };
  */
}

std::string to_string(const Slice &slice) {
  std::string end = slice.end() >= 0 ? ", " + std::to_string(slice.end()) : "";
  return "Slice(" + to_string(slice.dim()) + ", " +
         std::to_string(slice.begin()) + end + ")\n";
}

} // namespace scipp::core
