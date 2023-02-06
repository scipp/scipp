// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#include "scipp/variable/string.h"

#include <iomanip>
#include <set>

#include "scipp/core/format.h"
#include "scipp/core/string.h"
#include "scipp/core/tag_util.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

std::ostream &operator<<(std::ostream &os, const Variable &variable) {
  return os << to_string(variable);
}

std::string format_variable_compact(const Variable &variable) {
  const auto s = to_string(variable.dtype());
  if (variable.unit() == units::none)
    return s;
  else
    return s + '[' + variable.unit().name() + ']';
}

std::string to_string(const Variable &variable) {
  return core::FormatRegistry::instance().format(variable);
}

std::string to_string(const std::pair<Dim, Variable> &coord) {
  using units::to_string;
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
