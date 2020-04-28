// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include <tuple>
#include <string>

namespace scipp::python {

using strpair = const std::pair<const std::string, const std::string>;

class Docstring {

public:

  Docstring() = default;
  Docstring(const std::string description, const std::string raises, const std::string seealso, const std::string returns, const std::string rtype);
  Docstring(const std::string description, const std::string raises, const std::string seealso, const std::string returns, const std::string rtype, const std::vector<std::pair<std::string, std::string>> &params);
  Docstring operator~() const;

  const std::string description() {return m_description; };
  const std::string raises() {return m_raises; };
  const std::string seealso() {return m_seealso; };
  const std::string returns() {return m_returns; };
  const std::string rtype() {return m_rtype; };

  void with_out_arg();

  std::string to_string();

private:
  std::string m_description, m_raises, m_seealso, m_returns, m_rtype;
  std::vector<std::pair<std::string, std::string>> params;

};


} // namespace scipp::python
