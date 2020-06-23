// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Simon Heybrock
#pragma once

#include <map>
#include <string>

#include "scipp-variable_export.h"
#include "scipp/common/index.h"
#include "scipp/variable/variable.h"

namespace scipp::variable {

template <class Id, class Key, class Value> class ConstView;
template <class T, class U> class MutableView;

SCIPP_VARIABLE_EXPORT std::ostream &
operator<<(std::ostream &os, const VariableConstView &variable);
SCIPP_VARIABLE_EXPORT std::ostream &operator<<(std::ostream &os,
                                               const VariableView &variable);
SCIPP_VARIABLE_EXPORT std::ostream &operator<<(std::ostream &os,
                                               const Variable &variable);

SCIPP_VARIABLE_EXPORT std::string to_string(const Variable &variable);
SCIPP_VARIABLE_EXPORT std::string to_string(const VariableConstView &variable);
SCIPP_VARIABLE_EXPORT std::string
to_string(const std::pair<Dim, VariableConstView> &coord);
SCIPP_VARIABLE_EXPORT std::string
to_string(const std::pair<std::string, VariableConstView> &attr);

SCIPP_VARIABLE_EXPORT std::string
format_variable(const std::string &key, const VariableConstView &variable,
                const Dimensions &datasetDims = Dimensions());

/// Abstract base class for formatters for variables with element types not in
/// scipp-variable module.
class SCIPP_VARIABLE_EXPORT AbstractFormatter {
public:
  virtual ~AbstractFormatter() = default;
  virtual std::string format(const VariableConstView &var) const = 0;
};

/// Concrete class for formatting variables with element types not in
/// scipp-variable.
template <class T> class Formatter : public AbstractFormatter {
  std::string format(const VariableConstView &var) const override {
    return array_to_string(var.template values<T>());
  }
};

/// Registry of formatters.
///
/// Modules instantiating variables with custom dtype can call `emplace` to
/// register a formatter.
class SCIPP_VARIABLE_EXPORT FormatterRegistry {
public:
  void emplace(const DType key, std::unique_ptr<AbstractFormatter> formatter);
  bool contains(const DType key) const noexcept;
  std::string format(const VariableConstView &var) const;

private:
  std::map<DType, std::unique_ptr<AbstractFormatter>> m_formatters;
};

/// Return the global FormatterRegistry instrance
SCIPP_VARIABLE_EXPORT FormatterRegistry &formatterRegistry();

} // namespace scipp::variable
