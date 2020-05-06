// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "docstring.h"

Docstring& Docstring::description(const std::string s, const bool append) {
  if (append)
    m_description += s;
  else
    m_description = s;
  return *this;
}

Docstring& Docstring::raises(const std::string s, const bool append) {
  if (append)
    m_raises += s;
  else
    m_raises = s;
  return *this;
}

Docstring& Docstring::seealso(const std::string s, const bool append) {
  if (append)
    m_seealso += s;
  else
    m_seealso = s;
  return *this;
}

Docstring& Docstring::returns(const std::string s, const bool append) {
  if (append)
    m_returns += s;
  else
    m_returns = s;
  return *this;
}

Docstring& Docstring::rtype(const std::string s, const bool append) {
if (append)
    m_rtype += s;
  else
    m_rtype = s;
  return *this;
}

Docstring& Docstring::param(const std::string name, const std::string about) {
  if ( m_params.find(name) == m_params.end() ) {
    m_order.push_back(name);
  }
  m_params[name] = about;
  return *this;
}


const char * Docstring::c_str() {
  m_output = m_description + "\n";
  for (const auto name : m_order)
    m_output += ":param " + name + ": " + m_params[name] + "\n";
  if (m_raises.size() > 0)
    m_output += ":raises: " + m_raises + "\n";
  if (m_seealso.size() > 0)
    m_output += ":seealso: " + m_seealso + "\n";
  if (m_returns.size() > 0)
    m_output += ":return: " + m_returns + "\n";
  if (m_rtype.size() > 0)
    m_output += ":rtype: " + m_rtype;
  return m_output.c_str();
}
