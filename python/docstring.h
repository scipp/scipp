// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include <string>
#include <vector>
#include <map>


class Docstring {

public:
  Docstring() = default;
  // Docstring(const Docstring &) = default;

  Docstring& description(const std::string s) { m_description = s; return *this;};
  Docstring& raises(const std::string s) { m_raises = s; return *this; };
  Docstring& seealso(const std::string s) { m_seealso = s; return *this; };
  Docstring& returns(const std::string s) { m_returns = s; return *this; };
  Docstring& rtype(const std::string s) { m_rtype = s; return *this; };
  Docstring& param(const std::string name, const std::string about);

  // const std::string rtype() const { return m_rtype; };
  // strpair param(const scipp::index ind) const { return m_params[ind]; };
  // void set_description(const std::string description) {
  //   m_description = description;
  // };
  // void set_raises(const std::string raises) { m_raises = raises; };
  // void set_seealso(const std::string seealso) { m_seealso = seealso; };
  // void set_returns(const std::string returns) { m_returns = returns; };
  // // void set_rtype(const std::string rtype) { m_rtype = rtype; };
  // void set_param(const scipp::index ind, strpair param) {
  //   m_params[ind] = param;
  // };
  // void insert_param(const scipp::index ind, strpair param);

  // const Docstring with_out_arg();

  const char * c_str();

private:
  std::string m_description, m_raises, m_seealso, m_returns, m_rtype, m_output;
  std::vector<std::string> m_order;
  std::map<std::string, std::string> m_params;
};
