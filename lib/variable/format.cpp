// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#include "scipp/variable/format.h"

#include <iomanip>
#include <sstream>

#include "scipp/core/format.h"

#include "scipp/variable/variable.h"

namespace scipp::variable {
namespace {
VariableFormatSpec parse_spec(const core::FormatSpec &spec) {
  if (!spec.current().empty())
    throw std::runtime_error("parsing not implemented yet");
  return VariableFormatSpec{}.with_nested(spec.nested());
}

void insert_unit(std::ostringstream &os, const units::Unit &unit) {
  if (unit == units::none)
    os << "  " << std::setw(15) << "<no unit>";
  else
    os << "  " << std::setw(15) << '[' + unit.name() + ']';
}

core::FormatSpec make_nested_spec(const VariableFormatSpec &spec,
                                  const Variable &var) {
  auto nested = spec.nested;
  nested.unit = var.unit();
  return nested;
}

void insert_dims_labels(std::ostringstream &os, const Variable &variable,
                        const Sizes &container_sizes) {
  const auto &dims = variable.dims();
  if (dims.empty()) {
    os << "()";
    return;
  }
  os << '(';
  const char *sep = "";
  for (const auto &dim : dims.labels()) {
    os << sep << dim;
    if ((container_sizes.contains(dim) ? container_sizes[dim] : 1) + 1 ==
        dims[dim])
      os << " [bin-edge]";
    sep = ", ";
  }
  os << ')';
}

auto array_slices(const Variable &var, const scipp::index length) {
  const auto size = var.dims().volume();
  if (length >= size)
    return std::pair{std::pair{index{0}, size},
                     std::pair{index{-1}, index{-1}}};
  const std::pair left{index{0}, length / 2};
  const std::pair right{size - length / 2, size};
  return std::pair{left, right};
}

template <class Getter>
void insert_array(std::ostringstream &os, const Variable &var,
                  const Getter &get, const core::FormatSpec &spec,
                  const core::FormatRegistry &formatters) {
  const index length = 4;
  const auto [left, right] = array_slices(var, length);

  os << '[';
  bool first = true;
  for (index i = left.first; i < left.second; ++i) {
    if (first)
      first = false;
    else
      os << ", ";
    os << formatters.format(var.dtype(), get(var, i), spec);
  }

  if (var.dims().volume() > length) {
    if (!first)
      os << ", ";
    os << "...";
  }

  for (index i = right.first; i < right.second; ++i) {
    os << ", " << formatters.format(var.dtype(), get(var, i), spec);
  }

  os << ']';
}
} // namespace

std::string format_variable(const Variable &var, const VariableFormatSpec &spec,
                            const core::FormatRegistry &formatters) {
  std::ostringstream os;
  if (spec.show_type)
    os << "<scipp.Variable> ";
  if (!var.is_valid()) {
    os << "invalid variable";
    return os.str();
  }

  const auto nested_spec = make_nested_spec(spec, var);
  static const char *col_sep = "  ";
  if (!spec.container_sizes)
    os << var.dims() << col_sep;
  os << std::setw(9) << var.dtype();
  insert_unit(os, var.unit());
  if (spec.container_sizes) {
    os << col_sep;
    insert_dims_labels(os, var, spec.container_sizes.value());
  }
  os << col_sep;
  insert_array(
      os, var,
      [](const Variable &v, const scipp::index i) { return v.value_cref(i); },
      nested_spec, formatters);
  if (var.has_variances()) {
    os << col_sep;
    insert_array(
        os, var,
        [](const Variable &v, const scipp::index i) {
          return v.variance_cref(i);
        },
        nested_spec, formatters);
  }
  return os.str();
}

namespace {
auto format_variable_ = core::FormatRegistry::insert_global<Variable>(
    [](const Variable &value, const core::FormatSpec &spec,
       const core::FormatRegistry &registry) {
      return format_variable(value, parse_spec(spec), registry);
    });
} // namespace

} // namespace scipp::variable
