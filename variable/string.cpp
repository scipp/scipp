// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <set>

#include "scipp/core/string.h"
#include "scipp/core/tag_util.h"
#include "scipp/variable/string.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

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
    return core::array_to_string(var.template values<T>());
  }
};
template <class T> struct VariancesToString {
  static auto apply(const VariableConstView &var) {
    return core::array_to_string(var.template variances<T>());
  }
};

template <template <class> class Callable, class... Args>
auto apply(const DType dtype, Args &&... args) {
  // Note that formatting via registry ignores the Callable, but custom types
  // should typically not have variances, so Callable should always be
  // `ValuesToString` in this case.
  if (formatterRegistry().contains(dtype))
    return formatterRegistry().format(args...);
  return core::callDType<Callable>(
      std::tuple<double, float, int64_t, int32_t, std::string, bool,
                 event_list<double>, event_list<float>, event_list<int64_t>,
                 event_list<int32_t>, Eigen::Vector3d, Eigen::Matrix3d>{},
      dtype, std::forward<Args>(args)...);
}

std::string format_variable(const std::string &key,
                            const VariableConstView &variable,
                            const Dimensions &datasetDims) {
  if (!variable)
    return std::string(tab) + "invalid variable\n";
  std::stringstream s;
  const std::string colSep("  ");
  s << tab << std::left << std::setw(24) << key;
  s << colSep << std::setw(9) << to_string(variable.dtype());
  s << colSep << std::setw(15) << '[' + variable.unit().name() + ']';
  s << colSep << make_dims_labels(variable, datasetDims);
  s << colSep;
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

std::string to_string(const std::pair<Dim, VariableConstView> &coord) {
  using units::to_string;
  return to_string(coord.first) + ":\n" + to_string(coord.second);
}

std::string to_string(const std::pair<std::string, VariableConstView> &coord) {
  return coord.first + ":\n" + to_string(coord.second);
}

void FormatterRegistry::emplace(const DType key,
                                std::unique_ptr<AbstractFormatter> formatter) {
  m_formatters.emplace(key, std::move(formatter));
}

bool FormatterRegistry::contains(const DType key) const noexcept {
  return m_formatters.find(key) != m_formatters.end();
}

std::string FormatterRegistry::format(const VariableConstView &var) const {
  return m_formatters.at(var.dtype())->format(var);
}

FormatterRegistry &formatterRegistry() {
  static FormatterRegistry registry;
  return registry;
}

} // namespace scipp::variable
