// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "docstring.h"

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
