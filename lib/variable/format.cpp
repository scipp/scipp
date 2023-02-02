// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

#include "scipp/core/format.h"

#include <iomanip>
#include <sstream>

#include "scipp/variable/variable.h"

namespace scipp::variable {
namespace {
// TODO datasetSlices

void insert_unit(std::ostringstream &os, const units::Unit &unit) {
  if (unit == units::none)
    os << "  " << std::setw(15) << "<no unit>";
  else
    os << "  " << std::setw(15) << '[' + unit.name() + ']';
}

core::FormatSpec make_nested_spec(const core::FormatSpec &spec,
                                  const Variable &var) {
  auto nested = spec.nested();
  nested.unit = var.unit();
  return nested;
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

std::string format(const Variable &var, const core::FormatSpec &spec,
                   const core::FormatRegistry &formatters) {
  std::ostringstream os;
  os << "<scipp.Variable> ";
  if (!var.is_valid()) {
    os << "invalid variable";
    return os.str();
  }

  const auto nested_spec = make_nested_spec(spec, var);
  static const char *col_sep = "  ";
  os << var.dims() << col_sep;
  os << std::setw(9) << var.dtype();
  insert_unit(os, var.unit());
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

auto format_variable = core::FormatRegistry::insert_global<Variable>(
    [](const Variable &value, const core::FormatSpec &spec,
       const core::FormatRegistry &registry) {
      return format(value, spec, registry);
    });
} // namespace

} // namespace scipp::variable
