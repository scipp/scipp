// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet

namespace scipp::python {



Docstring::Docstring(const std::string description, const std::string raises, const std::string seealso, const std::string returns, const std::string rtype) : m_description(description), m_raises(raises), m_seealso(seealso), m_returns(returns), m_rtype(rtype) {};


Docstring::Docstring(const std::string description, const std::string raises, const std::string seealso, const std::string returns, const std::string rtype, const std::vector<std::pair<std::string, std::string>> &params) : m_description(description), m_raises(raises), m_seealso(seealso), m_returns(returns), m_rtype(rtype){
  for (auto &p : params)
    m_params.push_back(p);
};

void Docstring::with_out_arg(){
  m_description += " (in-place)";
  m_rtype += " (View)";
};

std::string Docstring::to_string() {

  std::string docstring = m_description + "\n";
  for (auto &p : m_params)
    docstring += ":param " + p.first + ": " + p.second +"\n";
  docstring += ":raises: " + m_raises + "\n";
  docstring += ":seealso: " + m_seealso + "\n";
  docstring += ":return: " + m_returns + "\n";
  docstring += ":rtype: " + m_rtype;
  return docstring;
}


} // namespace scipp::python
