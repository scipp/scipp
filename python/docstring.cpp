// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#include "docstring.h"

void Docstring::update(std::string &field, const std::string &s,
                       const bool append) {
  if (append)
    field += s;
  else
    field = s;
}

Docstring &Docstring::description(const std::string &s, const bool append) {
  update(m_description, s, append);
  return *this;
}

Docstring &Docstring::raises(const std::string &s, const bool append) {
  update(m_raises, s, append);
  return *this;
}

Docstring &Docstring::seealso(const std::string &s, const bool append) {
  update(m_seealso, s, append);
  return *this;
}

Docstring &Docstring::returns(const std::string &s, const bool append) {
  update(m_returns, s, append);
  return *this;
}

Docstring &Docstring::rtype(const std::string &s, const bool append) {
  update(m_rtype, s, append);
  return *this;
}

Docstring &Docstring::param(const std::string &name, const std::string &about,
                            const std::string &type) {
  if (m_params.find(name) == m_params.end()) {
    m_order.push_back(name);
  }
  m_params[name] = {about, type};
  return *this;
}

const char *Docstring::c_str() {
  m_output.clear();
  if (m_description.size() > 0)
    m_output = m_description + "\n\n";
  for (const auto &name : m_order) {
    m_output += ":param " + name + ": " + m_params[name].first + "\n:type " +
                name + ": " + m_params[name].second + "\n";
  }
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
