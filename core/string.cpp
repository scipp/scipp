// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <set>

#include "scipp/core/dimensions.h"
#include "scipp/core/slice.h"
#include "scipp/core/string.h"
#include "scipp/core/tag_util.h"
#include "scipp/core/variable.h"

namespace scipp::core {

std::ostream &operator<<(std::ostream &os, const Dimensions &dims) {
  return os << to_string(dims);
}

std::ostream &operator<<(std::ostream &os, const VariableConstView &variable) {
  return os << to_string(variable);
}

std::ostream &operator<<(std::ostream &os, const VariableView &variable) {
  return os << VariableConstView(variable);
}

std::ostream &operator<<(std::ostream &os, const Variable &variable) {
  return os << VariableConstView(variable);
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
  default:
    return "unregistered dtype";
  };
}

std::string to_string(const Slice &slice) {
  std::string end = slice.end() >= 0 ? ", " + std::to_string(slice.end()) : "";
  return "Slice(" + to_string(slice.dim()) + ", " +
         std::to_string(slice.begin()) + end + ")\n";
}

std::string make_dims_labels(const VariableConstView &variable,
                             const Dimensions &datasetDims) {
  const auto &dims = variable.dims();
  if (dims.empty())
    return "()";
  std::string diminfo = "(";
  for (const auto dim : dims.labels()) {
    diminfo += to_string(dim);
    if (datasetDims.contains(dim) && (datasetDims[dim] + 1 == dims[dim]))
      diminfo += " [bin-edges]";
    diminfo += ", ";
  }
  diminfo.resize(diminfo.size() - 2);
  diminfo += ")";
  return diminfo;
}

template <class T> struct ValuesToString {
  static auto apply(const VariableConstView &var) {
    return array_to_string(var.template values<T>());
  }
};
template <class T> struct VariancesToString {
  static auto apply(const VariableConstView &var) {
    return array_to_string(var.template variances<T>());
  }
};

template <template <class> class Callable, class... Args>
auto apply(const DType dtype, Args &&... args) {
  // Note that formatting via registry ignores the Callable, but custom types
  // should typically not have variances, so Callable should always be
  // `ValuesToString` in this case.
  if (formatterRegistry().contains(dtype))
    return formatterRegistry().format(dtype, args...);
  return callDType<Callable>(
      std::tuple<double, float, int64_t, int32_t, std::string, bool,
                 event_list<double>, event_list<float>, event_list<int64_t>,
                 event_list<int32_t>, Eigen::Vector3d, Eigen::Quaterniond>{},
      dtype, std::forward<Args>(args)...);
}

std::string format_variable(const std::string &key,
                            const VariableConstView &variable,
                            const Dimensions &datasetDims) {
  if (!variable)
    return std::string(tab) + "invalid variable\n";
  std::stringstream s;
  const auto dtype = variable.dtype();
  const std::string colSep("  ");
  s << tab << std::left << std::setw(24) << key;
  s << colSep << std::setw(9) << to_string(variable.dtype());
  s << colSep << std::setw(15) << '[' + variable.unit().name() + ']';
  s << colSep << make_dims_labels(variable, datasetDims);
  s << colSep;
  if (dtype == DType::PyObject)
    s << "[PyObject]";
  else
    s << apply<ValuesToString>(variable.data().dtype(), variable);
  if (variable.hasVariances())
    s << colSep << apply<VariancesToString>(variable.data().dtype(), variable);
  s << '\n';
  return s.str();
}

std::string to_string(const Variable &variable) {
  return format_variable(std::string("<scipp.Variable>"), variable);
}

std::string to_string(const VariableConstView &variable) {
  return format_variable(std::string("<scipp.VariableView>"), variable);
}

void FormatterRegistry::emplace(const DType key,
                                std::unique_ptr<AbstractFormatter> formatter) {
  m_formatters.emplace(key, std::move(formatter));
}

bool FormatterRegistry::contains(const DType key) const noexcept {
  return m_formatters.find(key) != m_formatters.end();
}

std::string FormatterRegistry::format(const DType key,
                                      const VariableConstView &var) const {
  return m_formatters.at(key)->format(var);
}

FormatterRegistry &formatterRegistry() {
  static FormatterRegistry registry;
  return registry;
}

} // namespace scipp::core
