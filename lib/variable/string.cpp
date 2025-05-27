// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include <chrono>
#include <iomanip>
#include <set>

#include "scipp/core/array_to_string.h"
#include "scipp/core/eigen.h"
#include "scipp/core/string.h"
#include "scipp/core/tag_util.h"
#include "scipp/variable/string.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

std::ostream &operator<<(std::ostream &os, const Variable &variable) {
  return os << to_string(variable);
}

namespace {

std::string make_dims_labels(const Variable &variable,
                             const std::optional<Sizes> datasetSizes) {
  const auto &dims = variable.dims();
  if (dims.empty())
    return "()";
  std::string diminfo = "(";
  for (const auto &dim : dims.labels()) {
    diminfo += to_string(dim);
    if (datasetSizes) {
      if ((datasetSizes->contains(dim) ? (*datasetSizes)[dim] : 1) + 1 ==
          dims[dim])
        diminfo += " [bin-edge]";
    }
    diminfo += ", ";
  }
  diminfo.resize(diminfo.size() - 2);
  diminfo += ")";
  return diminfo;
}

template <class T> struct ValuesToString {
  static std::string apply(const Variable &var) {
    if (var.ndim() == 0)
      return core::scalar_array_to_string(var.template values<T>(), var.unit());
    return core::array_to_string(var.template values<T>(), var.unit());
  }
};
template <class T> struct VariancesToString {
  static std::string apply(const Variable &var) {
    if constexpr (core::canHaveVariances<T>()) {
      if (var.ndim() == 0)
        return core::scalar_array_to_string(var.template variances<T>(),
                                            var.unit());
      return core::array_to_string(var.template variances<T>());
    } else
      return std::string{};
  }
};

template <template <class> class Callable, class... Args>
auto apply(const DType dtype, Args &&...args) {
  // Note that formatting via registry ignores the Callable, but custom types
  // should typically not have variances, so Callable should always be
  // `ValuesToString` in this case.
  if (formatterRegistry().contains(dtype))
    return formatterRegistry().format(args...);
  return core::callDType<Callable>(
      std::tuple<double, float, int64_t, int32_t, std::string, bool,
                 scipp::core::time_point, Eigen::Vector3d, Eigen::Matrix3d,
                 Variable, bucket<Variable>, scipp::index_pair, Eigen::Affine3d,
                 scipp::core::Quaternion, scipp::core::Translation>{},
      dtype, std::forward<Args>(args)...);
}
} // namespace

std::string format_variable_compact(const Variable &variable) {
  const auto s = to_string(variable.dtype());
  if (variable.unit() == sc_units::none)
    return s;
  else
    return s + '[' + variable.unit().name() + ']';
}

std::string format_variable(const Variable &variable,
                            const std::optional<Sizes> &datasetSizes) {
  if (!variable.is_valid())
    return "invalid variable\n";
  std::stringstream s;
  const std::string colSep("  ");
  if (!datasetSizes)
    s << to_string(variable.dims()) << colSep;
  s << std::setw(9) << to_string(variable.dtype());
  if (variable.unit() == sc_units::none)
    s << colSep << std::setw(15) << "<no unit>";
  else
    s << colSep << std::setw(15) << '[' + variable.unit().name() + ']';
  if (datasetSizes)
    s << colSep << make_dims_labels(variable, datasetSizes);
  s << colSep;
  s << apply<ValuesToString>(variable.dtype(), variable);
  if (variable.has_variances())
    s << colSep << apply<VariancesToString>(variable.dtype(), variable);
  return s.str();
}

std::string to_string(const Variable &variable) {
  return std::string("<scipp.Variable> ") + format_variable(variable);
}

std::string to_string(const std::pair<Dim, Variable> &coord) {
  using sc_units::to_string;
  return to_string(coord.first) + ":\n" + to_string(coord.second);
}

std::string to_string(const std::pair<std::string, Variable> &coord) {
  return coord.first + ":\n" + to_string(coord.second);
}

void FormatterRegistry::emplace(const DType key,
                                std::unique_ptr<AbstractFormatter> formatter) {
  m_formatters.emplace(key, std::move(formatter));
}

bool FormatterRegistry::contains(const DType key) const noexcept {
  return m_formatters.find(key) != m_formatters.end();
}

std::string FormatterRegistry::format(const Variable &var) const {
  return m_formatters.at(var.dtype())->format(var);
}

FormatterRegistry &formatterRegistry() {
  static FormatterRegistry registry;
  return registry;
}

} // namespace scipp::variable
