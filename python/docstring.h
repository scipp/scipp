// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
/// @file
/// @author Neil Vaytet
#pragma once

#include <map>
#include <string>
#include <vector>

#include "scipp/dataset/dataset.h"
#include "scipp/variable/variable.h"

using namespace scipp::variable;
using namespace scipp::dataset;

class Docstring {

public:
  Docstring() = default;
  // Docstring(const Docstring &) = default;

  Docstring &description(const std::string s, const bool append = false);
  Docstring &raises(const std::string s, const bool append = false);
  Docstring &seealso(const std::string s, const bool append = false);
  Docstring &returns(const std::string s, const bool append = false);
  Docstring &rtype(const std::string s, const bool append = false);
  // Docstring& rtype(const std::string s) { m_rtype = s; return *this; };
  Docstring &param(const std::string name, const std::string about);
  Docstring &clear();

  // template <class T>
  // Docstring& rtype(const T& t) {
  //   using InputType = decltype(t);
  //   if (std::is_same_v<InputType, Variable>)
  //     m_rtype = "Variable";
  //   else if (std::is_same_v<InputType, VariableView>)
  //     m_rtype = "VariableView";
  //   else if (std::is_same_v<InputType, DataArray>)
  //     m_rtype = "DataArray";
  //   else if (std::is_same_v<InputType, DataArrayView>)
  //     m_rtype = "DataArrayView";
  //   else if (std::is_same_v<InputType, Dataset>)
  //     m_rtype = "Dataset";
  //   else if (std::is_same_v<InputType, DatasetView>)
  //     m_rtype = "DatasetView";
  //   return *this;
  // }

  template <class T> Docstring &rtype() {
    // using T = decltype(t);
    if (std::is_same_v<T, Variable>)
      m_rtype = "Variable";
    else if (std::is_same_v<T, VariableView>)
      m_rtype = "VariableView";
    else if (std::is_same_v<T, DataArray>)
      m_rtype = "DataArray";
    else if (std::is_same_v<T, DataArrayView>)
      m_rtype = "DataArrayView";
    else if (std::is_same_v<T, Dataset>)
      m_rtype = "Dataset";
    else if (std::is_same_v<T, DatasetView>)
      m_rtype = "DatasetView";
    return *this;
  }

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

  const char *c_str();

private:
  std::string m_description, m_raises, m_seealso, m_returns, m_rtype, m_output;
  std::vector<std::string> m_order;
  std::map<std::string, std::string> m_params;
};
