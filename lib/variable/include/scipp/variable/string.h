// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <map>
#include <string>

#include "scipp-variable_export.h"
#include "scipp/common/index.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

template <class T> std::string format_variable_like(const T &obj) {
  auto s =
      "(dims=" + to_string(obj.dims()) + ", dtype=" + to_string(obj.dtype());
  if (obj.unit() != sc_units::none)
    s += ", unit=" + to_string(obj.unit());
  return s + ')';
}

SCIPP_VARIABLE_EXPORT std::ostream &operator<<(std::ostream &os,
                                               const Variable &variable);

SCIPP_VARIABLE_EXPORT std::string to_string(const Variable &variable);
SCIPP_VARIABLE_EXPORT std::string
to_string(const std::pair<Dim, Variable> &coord);
SCIPP_VARIABLE_EXPORT std::string
to_string(const std::pair<std::string, Variable> &attr);

SCIPP_VARIABLE_EXPORT std::string
format_variable(const Variable &variable,
                const std::optional<Sizes> &datasetSizes = std::nullopt);
SCIPP_VARIABLE_EXPORT std::string
format_variable_compact(const Variable &variable);

/// Abstract base class for formatters for variables with element types not in
/// scipp-variable module.
class SCIPP_VARIABLE_EXPORT AbstractFormatter {
public:
  virtual ~AbstractFormatter() = default;
  [[nodiscard]] virtual std::string format(const Variable &var) const = 0;
};

/// Concrete class for formatting variables with element types not in
/// scipp-variable.
template <class T> class Formatter : public AbstractFormatter {
  [[nodiscard]] std::string format(const Variable &var) const override;
};

/// Registry of formatters.
///
/// Modules instantiating variables with custom dtype can call `emplace` to
/// register a formatter.
class SCIPP_VARIABLE_EXPORT FormatterRegistry {
public:
  FormatterRegistry() = default;
  FormatterRegistry(const FormatterRegistry &) = delete;
  FormatterRegistry &operator=(const FormatterRegistry &) = delete;
  void emplace(const DType key, std::unique_ptr<AbstractFormatter> formatter);
  [[nodiscard]] bool contains(const DType key) const noexcept;
  [[nodiscard]] std::string format(const Variable &var) const;

private:
  std::map<DType, std::unique_ptr<AbstractFormatter>> m_formatters;
};

/// Return the global FormatterRegistry instance
SCIPP_VARIABLE_EXPORT FormatterRegistry &formatterRegistry();

} // namespace scipp::variable
